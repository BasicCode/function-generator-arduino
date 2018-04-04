/*
 * An interface for the AD9833 function generator. This utilised an LCD
 * screen / library of choice and 5 buttons (up, down, left, right, select)
 * to choice the output frequency and waveform of the device.
 * Tested on Lenoardo r3 Arduino board.
 * 
 * TODO: enforce frequency limits; stop going negative or more than max_frequency.
 * 
 */

#include <AD9833.h>
#include <Goldilox_uLCD.h>

//Pins
#define lcd_r_pin 2 //RESET pin for the LCD unit
#define sig_gen_f_pin 3 //SPI control pin for the AD9833
#define LED_pin 13 //Default LED pin
#define btn_wave_sel 9 //Buttons...
#define btn_up 7
#define btn_dn 6
#define btn_left 5
#define btn_right 8

//LEFT/RIGHT movement definitions
#define LEFT 0
#define RIGHT 1

//Some device constants
const uint16_t BG_colour = 0xFFFF; //default background colour
const double max_frequency = 12500; //Maximum output frequency in KHz

//Global variables
//Which wave is generated (See AD9833.h for definitions)
uint16_t waveType = TRIANGLE_WAVE;
//frequency cursor location (digits from right)
int freq_cursor = 3;
//The desired frequency to generate (in KHz)
uint32_t frequency = 100;

//The LCD library
Goldilox_uLCD uLCD;
//The signal generator
AD9833 ad9833;

void setup() {
  // put your setup code here, to run once:
  //Delay while the LCD stabilises
  delay(1000);
  
  //Initialise LCD
  uLCD.initLCD(&Serial1, 9600, lcd_r_pin);

  //Initialise the AD9833 function generator
  ad9833.begin(sig_gen_f_pin);
  
  //LED pin
  pinMode(LED_pin, OUTPUT);
  
  //Button set up
  pinMode(btn_wave_sel, INPUT);
  pinMode(btn_up, INPUT);
  pinMode(btn_dn, INPUT);
  pinMode(btn_left, INPUT);
  pinMode(btn_right, INPUT);

  //Simple test splash screen and test routine
  uLCD.putString(2, 2, 5, 0xFAAF, "Signal");
  uLCD.putString(2, 3, 4, 0xFAAF, "Generator");
  uLCD.BGColour(BG_colour);
  uLCD.clearLCD(); //Clear the test routine ready for normal operatin

  //Draw the screen components
  displayBackground();
  displayWaveforms();
  displayFrequency();
  
  //Start the AD9833 with the new frequency
  ad9833.setFrequency(waveType, frequency);
  
  //LED ON to indicate initialisation complete
  digitalWrite(LED_pin, HIGH);
}

/*
 * Main loop just detects button presses. There is no action
 * to perform other than respond to user input. The LCD and
 * AD9833 look after themselves. :)
 */
void loop() {
  //Wave select button has been pressed. Cycle to next waveform
  if(digitalRead(btn_wave_sel)) {
    nextWaveform();
    //Debounce
    delay(250);
  }

  //LEFT arrow button has been pressed, move cursor
  if(digitalRead(btn_left)) {
    moveFreqCursor(LEFT);
    //Debounce
    delay(250);
  }

  //RIGHT arrow button pressed, move cursor.
  if(digitalRead(btn_right)) {
    moveFreqCursor(RIGHT);
    //Debounce
    delay(250);
  }

  //UP button pressed, increase frequency
  if(digitalRead(btn_up)) {
    incrementFrequency();
    //short Debounce
    delay(100);
  }

  //DOWN button pressed, decrease frequency
  if(digitalRead(btn_dn)) {
    decrementFrequency();
    //short Debounce
    delay(100);
  }
}

/*
 * This function draws the basic title and lines for the screen
 * layout. This should be called first after refreshing the
 * screen, and then other items added on top.
 */
void displayBackground() {
  //Draw the Title
  uLCD.putString(1, 0, 0, 0xFAAF, "Signal Generator");
  uLCD.Line(0, 10, 128, 10, 0xAAAA);
  uLCD.Line(10, 45, 118, 45, 0xAAAA);
}

/*
 * Waveform selection display. Displays the three waveform types
 * with the selected type highlighted.
 */
void displayWaveforms() {
  //Clear out the waveform part of the display with a white box
  uLCD.Square(0, 0, 12, 128, 42, BG_colour);
  
  //Put a highlight box behind which ever waveform is slected
  switch(waveType) {
    case(SINE_WAVE):
      uLCD.Square(0, 4, 12, 124, 22, 0xFF0);
    break;
    case(TRIANGLE_WAVE):
      uLCD.Square(0, 4, 22, 124, 32, 0xFF0);
    break;
    case(SQUARE_WAVE):
      uLCD.Square(0, 4, 32, 124, 42, 0xFF0);
    break;
  }

  //Now place the text over the top
  uLCD.putString(1, 2, 1, 0xFA00, "Sine Wave");
  uLCD.putString(1, 3, 1, 0xFA00, "Triangle Wave");
  uLCD.putString(1, 4, 1, 0xFA00, "Square Wave");
}

/*
 * Display the frequency selection text and graphics with a digit selector
 * indication.
 */
void displayFrequency() {
  //cursor display location; including commas
  int display_location = freq_cursor;
  
  //Get a human-readable string of the frequency
  String freq_string = frequencyToString();
  
  //First clear the frequency part of the screen
  uLCD.Square(0, 0, 46, 128, 128, BG_colour);
  
  //print the text
  uLCD.putString(2, 6, 2, 0x00AF, freq_string);
  /*
  * Calculate where the marker should be. Each char is 8 px wide.
  */
  //Add the comma spaces first
  if(display_location > 2)
    display_location++;
  if(display_location > 6)
    display_location++;

  //Draw a happy little triangle above and below the digit that is selected
  //This is our cursor!
  int triangle_start = 128 - ((display_location * 8) + (4 * 8)) -8;
  int height = 8 * 8 + 2;
  uLCD.Triangle(0, triangle_start, height, triangle_start + 4, height + 4, triangle_start + 8, height, 0x0AC0);
  uLCD.Triangle(0, triangle_start + 4, height + 19, triangle_start, height + 23, triangle_start + 8, height + 23, 0x0AC0);
}

/*
 * Cycles to the next waveform in the list.
 * Called from the main loop when the wave select
 * button is pressed.
 */
void nextWaveform() {
  /*
   * Choose the next waveform based on which ever one is
   * currently selected. Then call the display update function.
   */
  switch(waveType) {
    case(SINE_WAVE):
      waveType = TRIANGLE_WAVE;
    break;
    case(TRIANGLE_WAVE):
      waveType = SQUARE_WAVE;
    break;
    case(SQUARE_WAVE):
      waveType = SINE_WAVE;
    break;
  }

  //Update the display with the new wave form
  displayWaveforms();

  //Update the device with the request
  ad9833.setFrequency(waveType, frequency);
}

/*
 * Moves the frequency cursor left or right. Called by the
 * main loop on button press and passed an int 0 = LEFT,
 * or int 1 = RIGHT.
 * 
 * (NOTE: we are working in KHz and so should not be able
 * to change the first three digits)
 */
void moveFreqCursor(int move_direction) {
  //Which direction are we moving?
  //Move left
  if(move_direction == LEFT) {
    //Check that we don't go further than there are characters.
    if(freq_cursor > 6)
      freq_cursor = 3;
    else
      freq_cursor++;
  }

  //Move RIGHT
  if(move_direction == RIGHT) {
    //Check bounds
    if(freq_cursor < 4)
      freq_cursor = 7;
    else
      freq_cursor--;
  }
  
  //Render the frequency display again
  displayFrequency();
}

/*
 * Increased the frequency by a a factor of 10x based on
 * the location of the cursor on screen.
 * For example change by 1000Hz when the 1000s column is
 * selected etc.
 */
void incrementFrequency() {
  /*
  * Because each cursor location is a 10x increase we
  * can just add the 1 x 10 ^ freq_cursor to the
  * frequency.
  * 
  * (Remember to subtract 10^3 because we are working in KHz)
  * 
  * NOTE: pow() returns a float that might not be exactly
  * the expected result so have to round up.
  * 
  * TODO: check output frequency is within bounds.
  */
  frequency += round(pow(10, freq_cursor-3));

  //Update the display
  displayFrequency();
  
  //Update the device with the new frequency
  ad9833.setFrequency(waveType, frequency);
}

/*
 * Decreases the frequency by a factor of 10x based
 * on the location of the cursor on screen.
 * 
 * (REMEMBER we are wworking in KHz, not Hz)
 */
void decrementFrequency() {
  /*
  * Decrement the frequency by a power of 10.
  * pow() returns a float and tends to return 999 
  * instead of 1000.
  * 
  * TODO: check output is within range supported
  * by the device.
  */
  frequency -= round(pow(10, freq_cursor-3));
  
  //Update the display
  displayFrequency();
  
  //Update the device with the new frequency
  ad9833.setFrequency(waveType, frequency);
}

/*
 * Convert the frequency variable to a human-readable string
 * of constant length, with commas, and Hz, ready for display.
 * 
 * The screen should look like this:
 * __XX,XXX,XXXHz__
 * 
 * I'm not really sure how to do this well, so it could get messy.
 */
String frequencyToString() {
  
  //Each line is 16 chars long
  char return_string[14];

  //Get a string of the digit
  sprintf(return_string, "%05d,000Hz", frequency);

  //Add a comma at the 3rd position from left, and shift everything right
  for(int i = sizeof(return_string); i > 1; i--) {
    return_string[i + 1] = return_string[i];
  }
  return_string[2] = ',';

  return return_string;
}

