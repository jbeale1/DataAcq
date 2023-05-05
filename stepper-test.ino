// Works OK using TMC2208 driver in "dumb" mode
// May 4 2023 JPB

#include <AccelStepper.h>

// defines pins numbers
const int stepPin = 8;
const int directionPin = 9;
const int enablePin = 10;

// Define a stepper and the pins it will use
// 1 or AccelStepper::DRIVER means a stepper driver (with Step and Direction pins)
AccelStepper stepper(AccelStepper::DRIVER, stepPin, directionPin);

void setup()
{
  Serial.begin(115200);
  delay(1500);

  stepper.setEnablePin(enablePin);
  stepper.enableOutputs();
  stepper.setMaxSpeed(2000);
  stepper.setSpeed(50);
}

int state = 0;

void loop()
{
  // stepper.runSpeed();
  int pos, v, a;

  if (stepper.distanceToGo() == 0)
    { 
      delay(1000);
      state = (state + 1) % 3;
      switch (state) {
        case 0:
          pos = (rand() % (3*4000));
          v = (rand() % 4000) + 1000;
          a = (rand() % 1000) + 500;
          break;
        case 1:
          pos = 0;
          v = 5000;
          a = 2000;
          break;
        case 2:
          pos = 12000;
          v = 5000;
          a = 2000;
          break;
      }
        
      Serial.print(pos);
      Serial.print(", ");
      Serial.print(v);
      Serial.print(", ");
      Serial.print(v);
      Serial.println();
      stepper.moveTo(pos);
      stepper.setMaxSpeed(v);
      stepper.setAcceleration(a);
    }
    
  stepper.run();
}
