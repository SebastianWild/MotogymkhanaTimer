#include <Arduino.h>
#include <Ultrasonic.h>

#define RANGERPIN 5
const int DELAY = 100;    // msec to delay between readings
Ultrasonic ultrasonic(RANGERPIN);

void setup() {
  // open serial communication
  Serial.begin(9600);
  Serial.print("\r\nDistance: ");
}

void loop() {
  char _buffer[7];
  int centimeters;

  centimeters = ultrasonic.MeasureInCentimeters();
  delay(DELAY);  // msec

  sprintf(_buffer, "%03u cm", centimeters);
  Serial.println(_buffer);  // print distance (in cm) on serial monitor
}