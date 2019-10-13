/***************************************************
Derived from library sample code Copyright (c) 2019 Luis Llamas (www.luisllamas.es)
 ****************************************************/

// Ramp motor speed up & down, fwd & back using BTS7960 full H bridge controller
// and Luis Llamas BTS7960 library for Arduino
// also measures optointerrupter flag signal timing to calculate motor RPM
// and Isense1,Isense2 output from H-bridge module (with RC filter 1k, 10uF)

// PINOUT
// L_EN -> 8
// R_EN -> 8
// L_PWM -> 9
// R_PWM -> 10
 
#include "BTS7960.h"

// pins 9,10 PWM => TIMER1
// TIMER0 => delay()

const uint8_t LED1 = 13;  // Arduino built-in LED
const uint8_t EN1 = 7;
const uint8_t EN2 = 8;
const uint8_t L_PWM = 9;
const uint8_t R_PWM = 10;

const byte interruptPin = 2;  // optointerrupter input
volatile byte state = LOW;

volatile unsigned long ucap = 0;  // millisecond capture
unsigned long olducap = 0;
int speed = 0;            // current motor speed
float vin1=0;             // ADC reading from PWM control I1 out
float vin2=0;           // ADC readings from PWM control I2 output
float f=0.005;             // ADC reading LP-filter smoothing fraction

BTS7960 motorController(EN1, EN2, L_PWM, R_PWM);

// ------------------------------------------------------
void setup() 
{
  Serial.begin(115200);

  pinMode(LED1, OUTPUT);
  Serial.begin(115200);
  delay(200);

  for (int i=0;i<2;i++) {  // blink 2x to show startup
    digitalWrite(LED1,HIGH);   
    delay(250);
    digitalWrite(LED1,LOW);  
    delay(250);
  }

  // PWM chip only spec'd for 25 kHz, but 31 kHz seems to work OK, and is much less annoying than 3.9 kHz
  
  TCCR1B = TCCR1B & B11111000 | B00000001;    // set timer 1 divisor to     1 for PWM frequency of 31372.55 Hz
  //  TCCR1B = TCCR1B & B11111000 | B00000010;    // set timer 1 divisor to 8 for PWM frequency of  3921.16 Hz

  Serial.println("sec,s1,s2,usec,pwm,rpm,ch1,ch2");
  Serial.println("# Motor speed test v1");

  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), edgeIn, FALLING); // measure only falling edge
  interrupts();

} // setup()


// ------------------------------------------------------
// Set the motor controller speed |s|<256.  + for CW, - for CCW
void setSpeed(int s) {
  if (s > 0) motorController.TurnRight(s);
  else motorController.TurnLeft(-s);
} // setSpeed()

// ------------------------------------------------------
// wtest(ms) : wait-while-testing-input
// wait 'ms' milliseconds while checking and reporting incoming edges
// note: if checkCap() finds an edge, significant extra delay for debounce & printing
void wTest(unsigned int ms) {
    for (unsigned int i=0; i<ms; i++) {
      checkCap();
      vin1 = (1.0-f)*vin1 + f*analogRead(A0);
      vin2 = (1.0-f)*vin2 + f*analogRead(A1);

      delay(1);
    }
  
} // wtest()

// ------------------------------------------------------


int sInc = 3;  // step size for motor speed
int wDelay = 4000;  // milliseconds to delay at each step
int minSpeed = 60;  // minimum PWM speed that gives any rotation at all
int maxSpeed = 226; // max effective PWM speed setting (larger isn't faster)

// delay for more-or-less fixed # of rotations at each speed
#define WT    wTest(int(wDelay*(50.0/(abs(speed)+10-minSpeed))))

void loop() 
{
  motorController.Enable();
  digitalWrite(LED1,HIGH);   

  for(speed = minSpeed ; speed < maxSpeed; speed+=sInc)  { // ramp up 0 to full
  	setSpeed(speed);
    Serial.print("# SPEED ");
    Serial.println(speed);
    WT;
  }  
  for(speed = maxSpeed ; speed > minSpeed; speed-=sInc)   { // ramp down full to 0
    setSpeed(speed);
    WT;
  }  
  motorController.Stop();
  digitalWrite(LED1,LOW);   
    WT;

  for(speed = -minSpeed ; speed > -maxSpeed; speed-=sInc)  { // ramp up 0 to full reverse
    setSpeed(speed);
    WT;
  }  
    WT;

  for(speed = -maxSpeed ; speed < -minSpeed; speed+=sInc)   {
    setSpeed(speed);
    WT;
  }  
  
  motorController.Stop(); // full braking force, motor shorted
  motorController.Disable();  // freewheel; motor open (High-Z)
  Serial.print("# END Motor Test ");
  Serial.println(millis()/1000);

  
} // end loop()
// ===========================================================================

// ------------------------------------------------------
// checkCap() : Test 'ucap' value to see if a new edge was captured.
// if so, print the delta-microseconds timing from previous edge,
// and re-arm interrupts for next edge

void checkCap() {
  if ((ucap != olducap) && (state == 0)) {
    // delay(20);
    long int dusec = ucap-olducap; // delta microseconds
    float rpm = 60.0 * 1E6 / dusec;
    Serial.print((float)ucap/1E6,3);
    Serial.print(", ");
    Serial.print(state);
    Serial.print(", ");
    Serial.print(digitalRead(interruptPin));
    Serial.print(", ");
    Serial.print(dusec);
    Serial.print(", ");
    Serial.print(speed);
    Serial.print(", ");
    Serial.print(rpm,4);
    Serial.print(", ");
    Serial.print(vin1,1);
    Serial.print(", ");
    Serial.print(vin2,1);
    Serial.println();
    olducap = ucap;
    interrupts();
  }
} // end checkCap()

// ------------------------------------------------------
// interrupt: capture microsecond timer at each input edge
void edgeIn() {
  noInterrupts();
  state = digitalRead(interruptPin);
  ucap = micros();
} // edgeIn()
