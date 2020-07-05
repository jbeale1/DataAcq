/*
Record relative timing (millis) of high & low edges on two inputs
J.Beale 4-July-2020
 */

// #include <digitalWriteFast.h>

// constants won't change. They're used here to 
// set pin numbers:
const int buttonPin1 = 2;     // digital input pin #1
const int buttonPin2 = 3;     // digital input pin #2

const int ledPin =  13;      // the number of the LED pin
const int ledPin2 =  10;      // the number of the LED pin
  // Pin 13: Arduino has an LED connected on pin 13
  // Pin 11: Teensy 2.0 has the LED on pin 11
  // Pin  6: Teensy++ 2.0 has the LED on pin 6
  // Pin 13: Teensy 3.0 has the LED on pin 13

// variables will change:
int button1State = 0;         // current value of input 1
int button1StateOld = 0;      // previous value of input 1
int button2State = 0;
int button2StateOld = 0;

unsigned int t1Now, t1Last;    // time of events in milliseconds since start
unsigned int t2Now, t2Last;    // time of events in milliseconds since start
unsigned int t1Low, t2Low;     // most recent time at falling edge on signal 1, 2
int t12Delta, t21Delta;

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("# Digital Input Reading v2  2020-07-4 JPB");

  pinMode(ledPin, OUTPUT);      
  pinMode(ledPin2, OUTPUT);      
  pinMode(buttonPin1, INPUT);     
  pinMode(buttonPin2, INPUT);     
  button1StateOld = digitalRead(buttonPin1);
  button2StateOld = digitalRead(buttonPin2);
  t1Last = millis();
  t2Last = t1Last;
  t1Low = t1Last;
  t2Low = t2Last;
  t12Delta = 0; 
  t21Delta = 0;
}

void loop(){
  button1State = digitalRead(buttonPin1);
  button2State = digitalRead(buttonPin2);

  if (button1State != button1StateOld) {
    t1Now = millis();
    unsigned int t1Delta = t1Now - t1Last;
    if (button1State == HIGH) {     
      digitalWrite(ledPin, LOW);  
      Serial.print("HIGH1 ");
    } 
    else {
      t1Low = t1Now;
      t12Delta = t1Low - t2Low;
      digitalWrite(ledPin, HIGH); 
      Serial.print("LOW1 ");
      Serial.print(t12Delta);
      Serial.print(" ");
    }
    Serial.println(t1Delta);
    button1StateOld = button1State; // remember current setting for next time
    t1Last = t1Now;
  }

  if (button2State != button2StateOld) {
    t2Now = millis();
    unsigned int t2Delta = t2Now - t2Last;
    if (button2State == HIGH) {     
      digitalWrite(ledPin2, LOW);  
      Serial.print("HIGH2 ");
    } 
    else {
      t2Low = t2Now;
      t21Delta = t2Low - t1Low;
      digitalWrite(ledPin2, HIGH); 
      Serial.print("LOW2 ");
      Serial.print(t21Delta);
      Serial.print(" ");
    }
    Serial.println(t2Delta);
    button2StateOld = button2State; // remember current setting for next time
    t2Last = t2Now;
  }

} // end loop()
