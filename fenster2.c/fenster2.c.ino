#include <Wire.h>
#include <VL53L0X.h>

// Initialize VL53L0X ToF sensor
#define HIGH_ACCURACY
VL53L0X sensor;

// Define L298N pin mappings
int IN1 = 7;
int IN2 = 8;

// Define 3 position slide switch pins
int IN3 = 2;
int IN4 = 3;

int mode = 0;

// 0 = closed
// 1 = open
int windowState = 0;

bool windowOpening = false;
bool windowClosing = false;
bool readyForAutomation = true;

bool automation_waiting = true;
bool automation_opening = false;
bool automation_airing = false;
bool automation_closing = false;


unsigned long eventTime;
unsigned long currentTime;

const unsigned long extendTime = 5000;
const unsigned long stayTime = 7000;
const unsigned long retreatTime = 5000;

void setup() {
  Serial.begin(9600);
  Wire.begin();

  sensor.setTimeout(500);
  if (!sensor.init())
  {
    Serial.println("Failed to detect and initialize sensor!");
    while (1) {}
  }

  sensor.setMeasurementTimingBudget(200000);
  // Start continuous back-to-back mode (take readings as
  // fast as possible).  To use continuous timed mode
  // instead, provide a desired inter-measurement period in
  // ms (e.g. sensor.startContinuous(100)).
  sensor.startContinuous();
  Serial.println("Initialized ToF sensor.");

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  pinMode(IN3, INPUT_PULLUP);
  pinMode(IN4, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(IN3) == LOW) {
    // Serial.println("I'm in mode 0 and am keeping the window closed.");
    mode = 0;
  } else if (digitalRead(IN4) == LOW){
    mode = 1;
  } else if (digitalRead(IN3) == HIGH && digitalRead(IN4) == HIGH){
    // Serial.println("I'm in mode 2! Window is operated automatically.");
    mode = 2;
  }

  if (mode == 0) {
    // close window if it's open
    if (windowState == 1){
      if (windowClosing == false){
        Serial.println("Closing window...");
        eventTime = millis();
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, HIGH);
        windowClosing = true;
        windowOpening = false;   
      } else if (windowClosing == true){
        currentTime = millis();
        if ((currentTime - eventTime) > extendTime) {
          Serial.println("Window closed.");
          windowState = 0;
          windowClosing = false;
          readyForAutomation = true;
        }
      }
    } else {
      readyForAutomation = true;
    }
  }
  if (mode == 1) {
    // open window if it's closed
    if (windowState == 0){
      if (windowOpening == false){
        Serial.println("Opening window...");
        eventTime = millis();
        windowState = 1;
        readyForAutomation = false;
        digitalWrite(IN1, HIGH);
        digitalWrite(IN2, LOW);
        windowOpening = true;
        windowClosing = false;   
      } else if (windowOpening == true){
        currentTime = millis();
        if ((currentTime - eventTime) > retreatTime){
          Serial.println("Window opened.");
          digitalWrite(IN1, LOW);
          digitalWrite(IN2, LOW);
          windowOpening = false;
        }
      } else {
        readyForAutomation = false;
      }
    }

  }
  if (mode == 2) {   
    if (readyForAutomation == false){
      if (windowOpening == true){
        Serial.println("Making sure the window is closed for automation");
        eventTime = millis();
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, HIGH);
        windowClosing = true;
        windowOpening = false;
      } else if (windowClosing == true){
        currentTime = millis();
        if ((currentTime - eventTime) > retreatTime){
          Serial.println("Window closed. ready for automation input");
          digitalWrite(IN1, LOW);
          digitalWrite(IN2, LOW);
          windowOpening = false;
          readyForAutomation = true;
        }
      }
       
    } else if (readyForAutomation == true) {
      if (automation_waiting == true){
        // waiting for ToF sensor input
        int distanceRead = sensor.readRangeContinuousMillimeters();
        if (sensor.timeoutOccurred()) {
          // this could switch on a LED displaying sensor conditions
          Serial.print(" TIMEOUT");
        } else {
          if (distanceRead < 1000) {
            automation_waiting = false;
            automation_opening = true;
          }
        }
      }
      if (automation_opening == true){
        // opening window
        if (windowOpening == false){
          Serial.println("Opening window... (phase 1 of automation)");
          eventTime = millis();
          windowState = 1;
          digitalWrite(IN1, HIGH);
          digitalWrite(IN2, LOW);
          windowOpening = true;
        } else if (windowOpening == true){
          currentTime = millis();
          if ((currentTime - eventTime) > retreatTime){
            Serial.println("Window opened. (phase 1 of automation completed)");
            digitalWrite(IN1, LOW);
            digitalWrite(IN2, LOW);
            windowOpening = false;
            automation_opening = false;
            automation_airing = true;
            eventTime = millis();
          }
        }
      }
      if (automation_airing == true){
        // waiting to air out the room
        currentTime = millis();
        if((currentTime - eventTime) > stayTime){
          Serial.println("Done airing the room. (phase 2 of automation completed)");
          automation_airing = false;
          automation_closing = true;
        }
      }
      if (automation_closing == true){
        // closing the window back up
        if (windowClosing == false){
          Serial.println("Closing window... (phase 3 of automation)");
          eventTime = millis();
          digitalWrite(IN1, LOW);
          digitalWrite(IN2, HIGH);
          windowClosing = true;
        } else if (windowClosing == true){
          currentTime = millis();
          if ((currentTime - eventTime) > extendTime){
            Serial.println("Window closed. (phase 3 of automation completed, waiting for another ToF input)");
            digitalWrite(IN1, LOW);
            digitalWrite(IN2, LOW);
            windowClosing = false;
            automation_closing = false;
            automation_waiting = true;
            windowState = 0;
          }
        }
      }
    }
   
  }

}
