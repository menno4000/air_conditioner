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

unsigned long eventTime;
unsigned long currentTime;


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
    eventTime = -900000;
    mode = 0;
  } else if (digitalRead(IN4) == LOW){
    eventTime = -900000;
    mode = 1;
  } else if (digitalRead(IN3) == HIGH && digitalRead(IN4) == HIGH){
    // Serial.println("I'm in mode 2! Window is operated automatically.");
    mode = 2;
  }

  if (mode == 0) {
    // close window if it's open
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    
  }
  if (mode == 1) {
    // KEEP WINDOW OPEN
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
   

  }
  if (mode == 2) {  
    int distanceRead = sensor.readRangeContinuousMillimeters();
    if (sensor.timeoutOccurred()) {
      // this could switch on a LED displaying sensor conditions
      Serial.print(" TIMEOUT");
    }  
         // waiting for ToF sensor input
    if (distanceRead < 200) {
      eventTime = millis();
    }
    currentTime = millis();

    if ((currentTime - eventTime) < 900000) {
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
    } else {
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
    }
    
  }

}
