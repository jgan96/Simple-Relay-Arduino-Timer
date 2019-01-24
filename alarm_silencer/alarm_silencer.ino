#include <Wire.h> // Enable this line if using Arduino Uno, Mega, etc.
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"

// Module connection pins (Digital Pins)
#define ONBUTTON 4
#define UPBUTTON 5
#define DOWNBUTTON 6
#define LED 13
#define RELAY 12

bool countdown = false;
bool sleep = false;
int t = 2; //time duration to disable alarm in minutes
int s = 0; //time duration of t in seconds
int secondsPassed = 0; //for the timer
int upButtonState = 0;         // current state of the button
int lastUpButtonState = 0;     // previous state of the button
int downButtonState = 0;         // current state of the button
int lastDownButtonState = 0;     // previous state of the button
int onButtonState = 0;         // current state of the button
int lastOnButtonState = 0;     // previous state of the button
long startMillis = 0;
long currentMillis = 0;
long previousMillis = 0;
long buttonHoldStartMillis = 0;
long sleepMillis = 0;
Adafruit_7segment matrix = Adafruit_7segment();

void toggleAlarm(bool disable) {
  //trigger relay & led
  if (disable)
  {
    digitalWrite(RELAY, HIGH);
    digitalWrite(LED, HIGH);
  }
  else
  {
    digitalWrite(RELAY, LOW);
    digitalWrite(LED, LOW);
  }
}

void resetTimer()
{
  toggleAlarm(false);
  countdown = false;

  //update display
  matrix.setBrightness(15);
  matrix.blinkRate(0);
  matrix.println(t * 100);
  matrix.drawColon(true);
  matrix.writeDisplay();

  //save last button press time
  sleepMillis = millis();
  sleep = false;
}

void updateClock(int secondsLeft)
{
  //update 7 segment display here
  Serial.print("seconds left: ");
  Serial.println(secondsLeft);

  int mm = floor(secondsLeft / 60);
  int ss = secondsLeft % 60;
  int mmss = (mm * 100) + ss;
  Serial.print(mm);
  Serial.print(":");
  if (ss < 10)
  {
    Serial.print("0");
    Serial.println(ss);
    //matrix.writeDigitNum(3, 0);
    //matrix.writeDigitNum(4, ss);
  }
  else
  {
    Serial.println(ss);
  }
  matrix.println(mmss);
  matrix.drawColon(true);
  matrix.writeDisplay();
}

void setup() {
  Serial.begin(9600);

  pinMode(ONBUTTON, INPUT_PULLUP); //pin for button
  pinMode(UPBUTTON, INPUT_PULLUP);
  pinMode(DOWNBUTTON, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  pinMode(RELAY, OUTPUT);

#ifndef __AVR_ATtiny85__
  Serial.println("7 Segment Backpack Test");
#endif
  matrix.begin(0x70);

  matrix.drawColon(true);
  matrix.writeDisplay();
  delay(1000);
  resetTimer();
}

void loop() {
  upButtonState = digitalRead(UPBUTTON);
  downButtonState = digitalRead(DOWNBUTTON);
  onButtonState = digitalRead(ONBUTTON);

  // compare the buttonState to its previous state
  if ((upButtonState != lastUpButtonState || downButtonState != lastDownButtonState) && !countdown)
  {
    // if the state has changed, increment the counter
    if (upButtonState == LOW) {
      // if the current state is HIGH then the button went from off to on:
      if (t < 99)
      {
        t++;
      }
      Serial.println("up pressed");
    }
    if (downButtonState == LOW) {
      // if the current state is HIGH then the button went from off to on:
      if (t > 1)
      {
        t--;
      }
      Serial.println("down pressed");
    }

    Serial.print("t minutes: ");
    Serial.println(t);
    buttonHoldStartMillis = millis();
    resetTimer();
    // Delay a little bit to avoid bouncing
    delay(50);
  }
  else if ((upButtonState == lastUpButtonState || downButtonState == lastDownButtonState) && onButtonState == HIGH && !countdown) //else if no buttons are being pressed or up/down buttons are being held
  {
    if (upButtonState == HIGH && downButtonState == HIGH) //sleep display after 5 minutes of no button presses
    {
      upButtonState = digitalRead(UPBUTTON);
      downButtonState = digitalRead(DOWNBUTTON);
      //Serial.println(sleepMillis);
      if (millis() - sleepMillis > 300000)
      {
        Serial.println(sleep);
        if (!sleep)
        {
          //slowly dim display
          sleep = true;
          for (int i = 15; i >= 0; i--)
          {
            matrix.setBrightness(i);
            delay(200);
          }
        }
        else
        {
          //after brightness goes to 0, clear display
          int8_t displayPos = 4;
          while (displayPos >= 0)
          {
            matrix.writeDigitRaw(displayPos--, 0x00);
          }
          matrix.writeDisplay();
        }
      }
    }
    else //else up/down buttons are being held
    {
      //Serial.println("button being held...");
    }
    while (upButtonState == LOW && downButtonState == HIGH)
    {
      upButtonState = digitalRead(UPBUTTON);
      if (millis() - buttonHoldStartMillis > 2000)
      {
        Serial.println("up button being held...");
        t = t + 5;
        if (t > 99)
        {
          t = 99;
        }
        resetTimer();
        Serial.print("t minutes: ");
        Serial.println(t);
        buttonHoldStartMillis = millis() - 1000; //increases increment speed after being held
      }
    }
    while (upButtonState == HIGH && downButtonState == LOW)
    {
      downButtonState = digitalRead(DOWNBUTTON);
      if (millis() - buttonHoldStartMillis > 2000)
      {
        Serial.println("down button being held...");
        t = t - 5;
        if (t < 1)
        {
          t = 1;
        }
        resetTimer();
        Serial.print("t minutes: ");
        Serial.println(t);
        buttonHoldStartMillis = millis() - 1000;
      }
    }
    delay(50); //debounce
  }
  else if (onButtonState != lastOnButtonState)
  {
    if (onButtonState == LOW)
    {
      if (!countdown)
      {
        //start timer
        Serial.println("starting timer...");
        startMillis = millis();
        s = t * 60;
        toggleAlarm(true);
        countdown = true;
      }
      else
      {
        //stop timer
        Serial.println("timer stopped!");
        resetTimer();
      }
    }
    delay(50); //debounce
  }
  else if (countdown) //in countdown
  {
    currentMillis = millis();
    if (currentMillis - startMillis < t * 60000) //if time passed has not exceeded the time set
    {
      secondsPassed = (int) round((currentMillis - startMillis) / 1000);
      //updateClock(s - secondsPassed); //secondsLeft = s - secondsPassed
      if (currentMillis - previousMillis > 1000) //update clock every second only
      {
        updateClock(s - secondsPassed); //secondsLeft = s - secondsPassed
        digitalWrite(LED, HIGH);
        matrix.drawColon(true);
        matrix.writeDisplay();
        //Serial.println("updating clock...");
        previousMillis = currentMillis;
      }
      else if (currentMillis - previousMillis > 500) //flash led and colon twice per second
      {
        digitalWrite(LED, LOW);
        matrix.drawColon(false);
        matrix.writeDisplay();
      }
    }
    else
    {
      //stop timer and reset
      digitalWrite(LED, HIGH);
      matrix.blinkRate(1);
      for (int i = 0; i <= 4; i++)
      {
        matrix.writeDigitNum(i, 0, false);
      }
      matrix.drawColon(true);
      matrix.writeDisplay();
      Serial.println("DONE!");
      delay(5000);
      resetTimer();
    }
  }
  /*else
    {
    resetTimer();
    }*/

  // save the current state as the last state, for next time through the loop
  lastUpButtonState = upButtonState;
  lastDownButtonState = downButtonState;
  lastOnButtonState = onButtonState;
}
