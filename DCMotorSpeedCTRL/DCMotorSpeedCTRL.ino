// GJB This Works
//BLK - 5V
//RST - D8
//DC  - D9
//CS  - D10
//SDA - D11
//SCK - D13
//VCC - 5V
//GND - GND

/**************************************************************************
  This is a library for several Adafruit displays based on ST77* drivers.

 **************************************************************************/
#include <SPI.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735

// These pins will also work for the 1.8" TFT shield.
#define TFT_CS        10 //GJB For Nano
#define TFT_RST        8 // Or set to -1 and connect to Arduino RESET pin
#define TFT_DC         9 //GJB for nano

#define MC1A        5  //PWM Pin for motor control
#define MC1B        6  //PWM Pin for motor control

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST); // This creates the screen object


//Rotary Encoder input including button
int outputA = 3;
int outputB = 4;
int button = 7;

// variables for the Rotary Encoder
int aState;
int oldState;
int bState;
int btnState;
int oldB = 0;

int mode = 0;               // 0 for selecting from the menu, 1 for setting the speed, 2 for setting the time
int sel = 0;                // rotary encoder count 0 - 15
int select = 0;             // menu selection based on 'sel' 
int oldselect = 0;          // copy of the last selection
int mspeed = 10;            // The motor speed. nominally in %
int rspeed = 0;             // The actual speed command to the motor (0-255) i.e. mspeed * 2.55
int runMins = 10;           // The minutes it will run for if AUTO mode is selected
bool am = true;             // True=Manual, False=AUTO
bool runStop = true;        // True=Stopped, False=Running
bool rev = true;            // True=Forward, False=Reverse

//Variables for the run timer
unsigned long oldMillis = 0;
unsigned long nowMillis = 0;
unsigned long aMin = 60000;  // 1 minute in milliseconds


int colorB = 0xF800;


// START OF SETUP =========================================
void setup(void) {
  Serial.begin(9600);
  Serial.print(F("Hello! ST77xx TFT Test"));
  // Use this initializer if using a 1.8" TFT screen:
  tft.initR(INITR_GREENTAB);      // Init ST7735S chip, black tab (GREENTAB seems to work better)

  Serial.println(F("Initialized"));

  pinMode(MC1A, OUTPUT);        // Setup the two PWM pins for thr motor
  pinMode(MC1B, OUTPUT);
  
  tft.fillScreen(ST77XX_BLACK); //Clear the screen to Black

  delay(50);
  tft.setRotation(2);           //Use the screen in portrait mode
  //lame joke intro display
  tft.fillScreen(0x0000);
  testdrawtext("Automatic", ST77XX_YELLOW, 10, 0);
  testdrawtext("Rotary", ST77XX_YELLOW, 10, 10);
  testdrawtext("Sand", ST77XX_YELLOW, 10, 20);
  testdrawtext("Equipment", ST77XX_YELLOW, 10, 30);
  testdrawtext("Metal", ST77XX_YELLOW, 10, 40);
  testdrawtext("Polisher", ST77XX_YELLOW, 10, 50);

  delay(5000);                  //Let the joke sink in for 5 seconds

// The contacts on the Rotary Encoder (The knob) switch to ground, so must be 'pulled up' when open
  pinMode(outputA, INPUT_PULLUP);
  pinMode(outputB, INPUT_PULLUP);
  pinMode(button, INPUT_PULLUP);
  oldState = digitalRead(outputA);
  
  redrawScreen();
}
//  END OF SETUP ======================================

//  MAIN LOOP =========================================
void loop() {

  // read the selector switch and the button
  btnState = digitalRead(button);
  aState = digitalRead(outputA);
  bState = digitalRead(outputB);

  // Actions dependant on mode
  if (mode == 0) {
    doMode0();        // Menu selection
  }
  if (mode == 1) {
    doMode1();        // Set speed
  }
  if (mode == 2) {
    doMode2();        // set mins
  }
  oldState = aState;

  doButtons(); // deal with any press of the button (The knob)

  //ACTUALLY CONTROL MOTOR
  if (!runStop)
  {
    rspeed=(mspeed*255)/100;
    if (rev)
    {
      analogWrite(MC1A, 0);       // forward
      analogWrite(MC1B, rspeed);
    }
    else
    {
      analogWrite(MC1A, rspeed);   // reverse
      analogWrite(MC1B, 0);
    }
  }
  else
  {
    analogWrite(MC1A, 0);       // Stop !!
    analogWrite(MC1B, 0);
    rspeed=0;
  }

  // TIMED RUN
  if (!runStop && !am && runMins > 0)   // if running in Auto and a valid run time has been set
  {
    nowMillis = millis();
    if (nowMillis - oldMillis >= aMin)
    {
      oldMillis = nowMillis;
      runMins--;                        // every minute decrement the RUN Mins number and update the screen
      showRunTime();
      if (runMins < 1)                  // When the timer reaches zero, stop the motor and update the display
      {
        runStop = true;
        testdrawtext("START", ST77XX_YELLOW, 25, 42);
      }
    }

  }
  else
  {
    oldMillis = millis();
  }
}
//   END OF MAIN LOOP ============================

//  PROCESS THE BUTTON PRESS
void doButtons()
{
  //Serial.print(btnState);
  //Serial.println(oldB);
  if (btnState == 0 && oldB == 1)
  {
    Serial.print(btnState);
    Serial.println(oldB);
    //AUTO MANUAL SELECTION
    if (mode == 0 && select == 0) {
      am = !am;
      tft.setTextSize(2);
      if (am) {
        testdrawtext("MANUAL", ST77XX_YELLOW, 25, 22);
      } else {
        testdrawtext("AUTO  ", ST77XX_GREEN, 25, 22);
      }
      doSquares();
      delay(100);
    }

    // RUN / STOP
    if (mode == 0 && select == 1) {
      runStop = !runStop;
      tft.setTextSize(2);
      if (runStop) {
        testdrawtext("START", ST77XX_YELLOW, 25, 42);
      } else {
        testdrawtext("STOP ", ST77XX_BLUE, 25, 42);
      }
      doSquares();
      delay(100);
    }

    // CHANGE TO MODE 1 AND SET SPEED
    if (select == 2)
    {
      if (mode == 0)
      {
        mode = 1;
        Serial.print(mode);
        //redrawScreen();
        drawarect(6, 14, 82, 14, ST77XX_GREEN);
        delay(200);
      }
      else
        // CHANGE BACK TO Mode 0
      {
        mode = 0;
        doSquares();
        delay(200);
      }
    }
    // CHANGE TO MODE 2 AND SET TIMED RUN
    if (select == 3)
    {
      if (mode == 0)
      {
        mode = 2;
        drawarect(6, 14, 122, 14, ST77XX_GREEN);
        delay(200);
      }
      else
        // CHANGE BACK TO Mode 0
      {
        mode = 0;
        doSquares();
        delay(200);
      }
    }
    // FORWARD  /  REVERSE
    if (mode == 0 && select == 4) {
      rev = !rev;
      tft.setTextSize(2);
      if (rev) {
        testdrawtext("FORWARD", ST77XX_YELLOW, 25, 142);
      } else {
        testdrawtext("REVERSE", ST77XX_BLUE, 25, 142);
      }
      doSquares();
      delay(100);
    }

    Serial.println(mode);

  }
  oldB = btnState;
}
//  MAIN SELECTION
void doMode0()
{
  if (mode == 0 && aState != oldState)
  {
    if (bState != aState)
    {
      sel++;
      if (sel > 15) {
        sel = 0;
      }
    }
    else
    {
      sel--;
      if (sel < 0) {
        sel = 15;
      }
    }
    // Serial.print("Selection = ");
    // Serial.println(sel);
    if (sel >= 0 && sel < 3) {
      select = 0;
    }
    if (sel >= 3 && sel < 6) {
      select = 1;
    }
    if (sel >= 6 && sel < 9) {
      select = 2;
    }
    if (sel >= 9 && sel < 12) {
      select = 3;
    }
    if (sel >= 12 && sel < 15) {
      select = 4;
    }

    if (oldselect != select && select == 0) {

      doSquares();

      oldselect = select;
    }
    if (oldselect != select && select == 1) {

      doSquares();

      oldselect = select;
    }
    if (oldselect != select && select == 2) {

      doSquares();

      oldselect = select;
    }
    if (oldselect != select && select == 3) {

      doSquares();

      oldselect = select;
    }
    if (oldselect != select && select == 4) {

      doSquares();

      oldselect = select;
    }
  }

}
// SET SPEED
void doMode1()
{
  if (aState != oldState)
  {
    if (bState != aState)
    {
      if (mspeed < 100) {
        mspeed += 5;
      }
    }
    else
    {
      if (mspeed > 5) {
        mspeed -= 5;
      }
    }
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    tft.setCursor(25, 82);
    tft.print(mspeed, 1);
    tft.print("  ");


  }
}


// SET TIMED RUN
void doMode2()
{
  if (aState != oldState)
  {
    if (bState != aState)
    {
      if (runMins < 120) {
        runMins += 1;
      }
    }
    else
    {
      if (runMins > 1) {
        runMins -= 1;
      }
    }
    showRunTime();
  }
}
// SHOW CURRENT RUN TIME
void showRunTime()
{
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setCursor(25, 122);
  tft.print(runMins, 1);
  tft.print("  ");
}
//REDRAW SELECTION RECTANGLES
void doSquares()
{
  drawarect(5, 16, 21, 140, ST77XX_BLACK);
  if (select == 0) {
    drawarect(6, 14, 22, 14, ST77XX_WHITE);
  }
  if (select == 1) {
    drawarect(6, 14, 42, 14, ST77XX_WHITE);
  }
  if (select == 2) {
    drawarect(6, 14, 82, 14, ST77XX_WHITE);
  }
  if (select == 3) {
    drawarect(6, 14, 122, 14, ST77XX_WHITE);
  }
  if (select == 4) {
    drawarect(6, 14, 142, 14, ST77XX_WHITE);
  }
}


// REFRESH MAIN SCREEN
void redrawScreen()
{
  tft.fillScreen(0x0000);
  tft.setTextSize(1);
  testdrawtext(" A.R.S.E.- M.P.", ST77XX_YELLOW, 15, 0);
  drawarect(0, 160, 12, 2, ST77XX_YELLOW);
  tft.setTextSize(2);
  if (am) {
    testdrawtext("MANUAL", ST77XX_YELLOW, 25, 22);
  } else {
    testdrawtext("AUTO  ", ST77XX_GREEN, 25, 22);
  }
  if (runStop) {
    testdrawtext("START", ST77XX_YELLOW, 25, 42);
  } else {
    testdrawtext("STOP ", ST77XX_BLUE, 25, 42);
  }

  testdrawtext("SPEED %", ST77XX_YELLOW, 25, 62);
  tft.setCursor(25, 82);
  tft.setTextColor(ST77XX_WHITE);
  tft.print(mspeed, 1);

  testdrawtext("RUN Mins", ST77XX_YELLOW, 25, 102);
  tft.setCursor(25, 122);
  tft.setTextColor(ST77XX_WHITE);
  tft.print(runMins, 1);
  if (rev) {
    testdrawtext("FORWARD", ST77XX_YELLOW, 25, 142);
  } else {
    testdrawtext("REVERSE", ST77XX_GREEN, 25, 142);
  }

}

// PRINT SOME TEXT at CURSOR X,Y
void testdrawtext(char *text, uint16_t color, int16_t x, int16_t y) {
  tft.setCursor(x, y);
  tft.setTextColor(color, ST77XX_BLACK);
  tft.setTextWrap(true);
  tft.print(text);
}

//DRAW A RECTANGLE
void drawarect(int16_t x1, int16_t x2, int16_t y1, int16_t y2, uint16_t color)
{
  tft.fillRect(x1, y1, x2, y2, color);
}
