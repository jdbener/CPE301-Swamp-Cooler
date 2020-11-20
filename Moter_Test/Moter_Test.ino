//www.elegoo.com
//2018.10.25

/*
  Stepper Motor Control - one revolution

  This program drives a unipolar or bipolar stepper motor.
  The motor is attached to digital pins 9 - 12 of the Arduino.
*/

#include <Stepper.h>

const int stepsPerRevolution = 2048;  // change this to fit the number of steps per revolution
const int rolePerMinute = 17;         // Adjustable range of 28BYJ-48 stepper is 0~17 rpm

// initialize the stepper library on pins 9 through 12:
Stepper myStepper(stepsPerRevolution, 9, 11, 10, 12);

void setup() {
  myStepper.setSpeed(rolePerMinute);
  // initialize the serial port:
  Serial.begin(9600);
}

void loop() {  
  // step one revolution  in one direction:
  myStepper.step(rolePerMinute);

}
