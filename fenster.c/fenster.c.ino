#include <Wire.h>
#include <VL53L0X.h>

// Initialize VL53L0X ToF sensor
#define HIGH_ACCURACY
VL53L0X sensor;

// Define L298N pin mappings
int IN1 = 7;
int IN2 = 8;

void setup()
{
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

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  cli();               // stop interrupts for settings
  TCCR1A = 0;          // clear timer 1 registers
  TCCR1B = 0;
  TCCR1A |= B00000100; // set prescalar to 256
  TIMSK1 |= B00000010; // enable compare match on register A
  OCR1A = 12500;       // set timer to trigger at 200ms
  sei();               // re-enable interrupts

  // state for the window automaton controlled by three way switch
  // 0: window is kept closed
  // 1: window is kept open
  // 2: window is waiting for ToF input
  int automationState = 1;
  // state to keep track of automation phases 
  // 0: window needs to be opened
  // 1: window needs to rest
  // 2: window needs to close
  // 3: window is waiting for more input
  int automationPhase = 3;
  
  // state for motor controller
  // 0: motor is retreating
  // 1: motor is resting
  // 2: motor is extending
  int motorState = 1;

  // state of the window
  // 0: window is closed
  // 1: window is open
  int windowState = 0;
  
  // time for window operations in frames of 200ms
  const int retreatTime = 15;
  const int extendTime = 15;
  const int airTime = 15;

  // counter for the window operations 
  int windowCounter = 0;
  // routine activity flag for window operations
  bool isRunning = false;
}

void loop()
{

  ISR(TIMER1_COMPA_vect) {
    TCNT1 = 0;            // reset timer 1
    if (automationState == 3) {
      // assure that window is closed
      // maybe this could trigger a warning LED
      if (windowState != 0) {
        windowCounter++;
        if (motorState != 2) {
          if (windowCounter =< extendTime) {
            digitalWrite(IN1, LOW);
            digitalWrite(IN2, HIGH);
            motorState = 2;
          }
          else {
            digitalWrite(IN1, LOW);
            digitalWrite(IN2, LOW);
            motorState = 1;
            windowState = 0;
            windowCounter = 0;
          }
        }
      } 
      // regular ToF sensor operation
      else {
        int distanceRead = sensor.readRangeContinuousMillimeters();
        if (sensor.timeoutOccurred()) {
          // this could switch on a LED displaying sensor conditions
          Serial.print(" TIMEOUT");
        } else {
          Serial.println(distanceRead);
          // run routine once ToF sensor is triggered, but only as long as no routine is in progress
          if (distanceRead < 1000) {
            if (automationPhase == 3) {
              Serial.println("poop time");
              automationPhase = 0;
            }
            isRunning = true;
          }
        }
        // routine for air conditiong
        if (isRunning == true) {
          windowCounter++;
          // window opening
          if (automationPhase == 0) {
            if (windowCounter =< retreatTime) {
              if (motorState != 0) {
                digitalWrite(IN1, LOW);
                digitalWrite(IN2, HIGH);
                motorState = 0;
              }
            } else {
              digitalWrite(IN1, LOW);
              digitalWrite(IN2, LOW);
              motorState = 1;
              automationPhase = 1;
              windowCounter = 0;
            }
          }
          // window resting
          if (automationPhase == 1) {
            if (windowCounter == airTime) {
              automationPhase = 2;
              windowCounter = 0;
            }
          }
          // window closing
          if (automationPhase == 2) {
            if (windowCounter =< extendTime) {
              if (motorState != 2) {
                digitalWrite(IN1, HIGH);
                digitalWrite(IN2, LOW);
                motorState = 2;
              }
            } else {
              digitalWrite(IN1, LOW);
              digitalWrite(IN2, LOW);
              motorState = 1;
              automationPhase = 3;
              windowCounter = 0;
              isRunning = false;
            }
          }
        }
      }    
    } 
    // control override state = closed
    else if (automationState == 0) {
       if (windowState != 0) {
        windowCounter++;
        if (motorState != 2) {
          if (windowCounter =< extendTime) {
            digitalWrite(IN1, LOW);
            digitalWrite(IN2, HIGH);
            motorState = 2;
          }
          else {
            digitalWrite(IN1, LOW);
            digitalWrite(IN2, LOW);
            motorState = 1;
            windowState = 0;
            windowCounter = 0;
          }
        }
      } 
    } 
    // controlled override state: open
    else if (automationState == 1) {
        if (windowState != 0) {
        windowCounter++;
        if (motorState != 2) {
          if (windowCounter =< extendTime) {
            digitalWrite(IN1, HIGH);
            digitalWrite(IN2, LOW);
            motorState = 2;
          }
          else {
            digitalWrite(IN1, LOW);
            digitalWrite(IN2, LOW);
            motorState = 1;
            windowState = 0;
            windowCounter = 0;
          }
        }
      } 
    }
  }


  {
    Serial.print("pooptime \n");
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    delay(3000);
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    //delay(900000);
    delay(2000);
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    delay(3000);
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    Serial.print("heat time \n");
  }
  delay(200);
}
