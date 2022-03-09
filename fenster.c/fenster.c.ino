#include <Wire.h>
#include <VL53L0X.h>

#define HIGH_ACCURACY
VL53L0X sensor;

//Define L298N pin mappings
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
}

void loop()
{
  int distanceRead = sensor.readRangeContinuousMillimeters();
  if (sensor.timeoutOccurred()) { Serial.print(" TIMEOUT"); }
  Serial.println(distanceRead);
  if (distanceRead < 8190) { 
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
