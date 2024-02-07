// Count input pulses in assigned period
// period: 60k milliseconds => counts per minute
// Arduino / Teensy 4.1  J.Beale 6-Feb-2024

byte digitalPinPulse = 5;  // Arduino Digital Pin 5 (D5)
byte boardLED = 13;          // onboard LED
volatile unsigned long pulseCount;
byte ledStatus = 0;

// period is number of milliseconds to accumulate pulses
unsigned long period;
unsigned long msecTarget;
unsigned long loopCnt = 0;
float f = 0.002;
float pfilt = 60.0 * 41;  // assume this starting value

void Isr ()    // count one pulse per leading edge
{
    pulseCount++;
}

void setup () {
  Serial.begin (115200);
  pinMode (boardLED, OUTPUT);
  pinMode (digitalPinPulse, INPUT);
  attachInterrupt (digitalPinToInterrupt (digitalPinPulse), Isr, RISING);

  period = 60000;  // milliseconds interval between readouts
  // period = 10000;  // milliseconds interval between readouts
  //period = 1000;  // milliseconds interval between readouts

  delay(100);
  msecTarget = millis() + period;
  Serial.print("GeigerLog 0.1 Start");
  Serial.println();
}

void loop () {
    unsigned long msec = millis();
    if (msec > msecTarget)  {
        msecTarget += period;
        unsigned long pulseOut = pulseCount;
        pfilt = pfilt * (1.0-f) + (f * pulseCount);
        pulseCount = 0;
        loopCnt++;
        Serial.print(loopCnt);
        Serial.print(", ");
        Serial.print(pulseOut);
        Serial.print(", ");
        Serial.println(pfilt,3);
        ledStatus = !ledStatus;
        digitalWrite(boardLED, ledStatus);
    }
    delay(10);
}
