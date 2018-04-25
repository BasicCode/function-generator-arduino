
/**
 * This is a program to drive the AD9833 function generator as a simple
 * signal generator. It used an LCD or OLED screen of your choice
 * (in this case the SSD1306 OLED driver) to display a frequency selection
 * and waveform selection.
 * 
 * The functions should be easy enough to customise with an LCD driver of
 * your choice.
 */

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <AD9833.h>


//Pre-defines constants
#define OLED_RESET 5 //Doesn't seem to be used
#define SIG_GEN_RESET 2
#define LED_PIN 13
#define BTN_SEL 5
#define BTN_LEFT 8
#define BTN_RIGHT 9
#define BTN_UP 6
#define BTN_DOWN 7

//Devices
Adafruit_SSD1306 display(OLED_RESET);
AD9833 ad9833;

//Global state variables
int cursor_location = 2; //Frequency select cursor position from right
int frequency = 100; //Frequency in KHz
uint16_t waveType = TRIANGLE_WAVE;

void setup() {

  //Initialise the display at I2C address 0x3C.
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.display(); //Displays the current image buffer.
  delay(1000);
  display.clearDisplay();

  //Initialse the signal generator
  ad9833.begin(SIG_GEN_RESET);
  ad9833.setFrequency(waveType, frequency);

  //Initialise pins
  pinMode(LED_PIN, OUTPUT);
  pinMode(BTN_SEL, INPUT);
  pinMode(BTN_LEFT, INPUT);
  pinMode(BTN_DOWN, INPUT);
  pinMode(BTN_UP, INPUT);

  digitalWrite(LED_PIN, HIGH);

  //Show the default display
  standardDisplay();
}

void loop() {
  //Wait for a button press
  /*
   * Left arror pressed. Move cursor left.
   */
  if(digitalRead(BTN_LEFT)) {
    //Move the cursor
    cursor_location++;
    //Check that the cursor is still within bounds
    if(cursor_location > 4)
      cursor_location = 4;
    //Refresh the display
    standardDisplay();
    //Debounce
    delay(250);
  }

  /*
   * Right arrow pressed. Move cursor right.
   */
  if(digitalRead(BTN_RIGHT)) {
    //Move the cursor
    cursor_location--;
    //Check within bounds
    if(cursor_location < 0)
      cursor_location= 0;
    //update the display
    standardDisplay();
    //Debounce
    delay(250);
  }

  /*
   * Up button pressed. Increase frequency.
   */
  if(digitalRead(BTN_UP)) {
    //Increase Frequency
    incrementFrequency();
    //Update the display
    standardDisplay();
    //Debounce
    delay(100);
  }

  /*
   * Down button pressed. Decrease frequency.
   */
  if(digitalRead(BTN_DOWN)) {
    //Decrease Frequency
    decrementFrequency();
    //Update the display
    standardDisplay();
    //Short debounce
    delay(100);
  }

  /*
   * Select button pressed.
   * A short press will cycle to the next waveform.
   * A long press will change the frequency range High / Low range.
   */
  if(digitalRead(BTN_SEL)) {
    //Cycle to next waveform
    nextWaveform();
    //Update the display
    standardDisplay();
    //Debounce
    delay(250);
  }
}

/*
 * Make the standard display routine.
 * This function is called whenever the display neeeds to
 * be updated. It is responsible for calling any other functions
 * required.
 * 
 * It clears, and re-draws the entire display which works on the
 * SSD1306, however on some slower LCD drivers it is worth only
 * blacking-out the part of the display you wish to re-draw.
 */
void standardDisplay() {
  //Clear the display
  display.clearDisplay();
  
  //Print the wave-form type 
  drawWaveform();

  //Draw the current frequency on screen
  drawFrequency();

  //Draw the current cursor on screen
  drawCursor();

  display.display();
}

/*
 * Draws the frequency unit select cursor on screen.
 * This function also draws the frequency mode icon,
 * just because it had to go somewhere.
 * 
 * Only to be called form the standardDisplay() function.
 */
void drawCursor() {
  int actual_cursor = cursor_location;
  //Add an extra space for the comma if needed
  if(actual_cursor > 2)
    actual_cursor++;
  
  //Calculate the cursor locatioin
  //Digits are 12 px wide + 4px left margin + 48px of text
  int left = 128 - (actual_cursor * 12) - 64;
  int top = 10;

  //Draw the cursor as a triangle above the digit in quesion
  display.fillTriangle(left, top, left + 8, top, left + 4, top + 4, WHITE);
}

/*
 * Draws the current frequency to the display buffer in nice,
 * human-readable format of the form;
 * "XX,XXX_KHz"
 * 
 * Should only be called from the standardDisplay() function.
 */
void drawFrequency() {
  //Each line is 16 chars long
  char frequency_string[11];

  //Get a string of the digit
  sprintf(frequency_string, "%05d KHz", frequency);

  //Add a comma at the 3rd position from left, and shift everything right
  for(int i = sizeof(frequency_string); i > 1; i--) {
    frequency_string[i + 1] = frequency_string[i];
  }
  frequency_string[2] = ',';

  //Add the new string to the display buffer
  display.setTextSize(2);
  display.setCursor(4, 16);
  display.println(frequency_string);
}

/*
 * Draws the current wave-type to the display buffer.
 * 
 * This should only be called by the standardDisplay() function,
 */
 void drawWaveform() {
   //Initial location and font size
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  //Now draw the selected string. Hard-coded for simplicity. :-)
  if(waveType == TRIANGLE_WAVE)
    display.println("<   TRIANGLE WAVE   >");
  if(waveType == SINE_WAVE)
    display.println("<     SINE WAVE     >");
  if(waveType == SQUARE_WAVE)
    display.println("<    SQUARE WAVE    >");
 }

/*
 * Increments the frequency by an value based on the cursor location.
 * Writes the result back to the global frequency register and
 * updates the AD9833.
 */
void incrementFrequency() {
  int old_frequency = frequency; //Fallback in case of error

  /*
   * Increase the frequency by a power of 10 for each cursor position.
   * NOTE: pow() returns a float which is not always exactly the value
   * expected so we need to round up.
   */
  frequency += round(pow(10, cursor_location));

  //Check bounds
  if(frequency > 12500) {
    //Revert to old frequency
    frequency = old_frequency;

    //Flash in the user's face
    errorFlash();
  }

  //Uupdate the AD9833 with the new frequency
  ad9833.setFrequency(waveType, frequency);
}

/*
 * Decreases the frequency by a power of 10 depending on the cursor
 * position.
 * Updates the global frequency register, and updates the AD9833.
 */
void decrementFrequency() {
  //Fallback value
  int old_frequency = frequency;
  
  /*
   * Decrease frequency by the appropriate power of 10.
   * NOTE: pow() returns a float that may be exactly correct.
   */
   frequency -= round(pow(10, cursor_location));

   //Check bounds
   if(frequency < 1) {
    //Revert to original frequency
    frequency = old_frequency;

    //Flash to indicate error
    errorFlash();
   }

   //Update the AD9833 with the new value
   ad9833.setFrequency(waveType, frequency);
}

/*
 * Notify the user of an invalid input by flashing the display briefly
 */
void errorFlash() {
  //Inverting the display briefly should be pretty alerting
  display.invertDisplay(1);
  delay(250);
  display.invertDisplay(0);
  
}

/*
 * Cycles to the next wave form in a list and updates the waveform
 * register, also updates the AD9833 unit.
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

  //Update the device with the request
  ad9833.setFrequency(waveType, frequency);
}

