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

// state for the window automaton controlled by three way switch
// 0: window is kept closed
// 1: window is kept open
// 2: window is waiting for ToF input
int automationState = 2;
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
volatile int windowCounter;
// routine activity flag for window operations
bool isRunning = false;

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
  Serial.println("Initialized ToF sensor.");

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  pinMode(IN3, INPUT_PULLUP);
  pinMode(IN4, INPUT_PULLUP);

  Serial.println("Setting timer.");

  cli();               // stop interrupts for settings
  TCCR1A = 0;          // clear timer 1 registers
  TCCR1B = 0;
  TCNT1 = 0;
  // TCCR1A |= B00000100; // set prescalar to 256
  // TIMSK1 |= B00000010; // enable compare match on register A
  OCR1A = 12500;       // set timer to trigger at 200ms

  TCCR1B |= (1 << WGM12);
  // Set CS10 and CS12 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);  
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  sei();               // re-enable interrupts

  Serial.println("timer set.");
}

void stateMachine()
{
  if (automationState == 3) {
    // assure that window is closed
    // maybe this could trigger a warning LED
    if (windowState != 0) {
      if (motorState != 2) {
        if (windowCounter < extendTime) {
          digitalWrite(IN1, LOW);
          digitalWrite(IN2, HIGH);
          motorState = 2;
        }
        else {
          digitalWrite(IN1, LOW);
          digitalWrite(IN2, LOW);
          motorState = 1;
          windowState = 0;
          windowCounter = extendTime+1;
          automationState = 2;
        }
      }
    } 
  }
    // regular ToF sensor operation
  if (automationState == 2){
     if (isRunning == false) {
      // Serial.println("Waiting for a shit!");
      int distanceRead = sensor.readRangeContinuousMillimeters();
      if (sensor.timeoutOccurred()) {
        // this could switch on a LED displaying sensor conditions
        Serial.print(" TIMEOUT");
      } else {
        // Serial.println(distanceRead);
        // run routine once ToF sensor is triggered, but only as long as no routine is in progress
        if (distanceRead < 1000) {
          if (automationPhase == 3) {
            // Serial.println("poop time");
            automationPhase = 0;
            isRunning = true;
            // Serial.println("Opening dat dere window");
          } else {
            // Serial.println("Window already in motion");
          }
        }
      }
      windowCounter = 0;
      // routine for air conditiong
    } else {
      // window opening
      if (automationPhase == 0) {
        if (windowCounter < retreatTime) {
          if (motorState != 0) {
            digitalWrite(IN1, LOW);
            digitalWrite(IN2, HIGH);
            motorState = 0;
            windowState = 1;
          }
        } else {
          //Serial.println("We breezy!");
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
          //Serial.println("We done here!");
          automationPhase = 2;
          windowCounter = 0;
          //Serial.println("Closing the window.");
        }
      }
      // window closing
      if (automationPhase == 2) {
       
        if (windowCounter < extendTime) {
          if (motorState != 2) {
            digitalWrite(IN1, HIGH);
            digitalWrite(IN2, LOW);
            motorState = 2;
          }
        } else {
          Serial.println("We dun here!");
          digitalWrite(IN1, LOW);
          digitalWrite(IN2, LOW);
          windowState = 0;
          motorState = 1;
          automationPhase = 3;
          windowCounter = 0;
          isRunning = false;
          //Serial.println("Waiting for the next shit...");
        }
      }
    }
  }    
   
  // control override state = closed
  // TODO add reaction to button
  if (automationState == 0) {
     if (windowState != 0) {
      if (motorState != 2) {
        if (windowCounter < extendTime) {
          digitalWrite(IN1, LOW);
          digitalWrite(IN2, HIGH);
          motorState = 2;
        }
        else {
          digitalWrite(IN1, LOW);
          digitalWrite(IN2, LOW);
          motorState = 1;
          windowState = 0;
          windowCounter = extendTime+1;
        }
      }
    } else { 
      windowCounter = 0;
    }
  } 
  // controlled override state: open
  // TODO add reaction to button
  if (automationState == 1) {
      if (windowState != 1) {
      if (motorState != 2) {
        if (windowCounter < retreatTime) {
          digitalWrite(IN1, HIGH);
          digitalWrite(IN2, LOW);
          motorState = 2;
          windowState = 1;
        }
        else {
          digitalWrite(IN1, LOW);
          digitalWrite(IN2, LOW);
          motorState = 1;
          windowCounter = retreatTime+1;
        }
      }
    } else { 
      windowCounter = 0;
    }
  }
}

void loop()
{
  if (digitalRead(IN3) == LOW) {
    // I'm in mode 0 and am keeping the window closed
    automationState = 0;
    isRunning = false;
  } else if (digitalRead(IN4) == LOW){
    //Serial.println("I'm in mode 2! Window is operated by the chip.");
    if (windowState == 1){
      automationState = 3;
    }
    else {
      automationState = 2;
    }
  } else if (digitalRead(IN3) == HIGH && digitalRead(IN4) == HIGH){
    // I'm in mode 1 and am keeping the window open
    automationState = 1;
    isRunning = false;
  }
  stateMachine();
}


ISR(TIMER1_COMPA_vect) {
  TCNT1 = 0;            // reset timer 1
  windowCounter ++;
  Serial.println("timer moment.");
  Serial.print("windowState: ");
  Serial.println(windowState);
  Serial.print("windowCounter: ");
  Serial.println(windowCounter);
  Serial.print("automationState: ");
  Serial.println(automationState);
  Serial.print("automationPhase: ");
  Serial.println(automationPhase);
 

}
