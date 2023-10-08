// Count input pulses in assigned period
// period: 60k milliseconds => counts per minute

byte digitalPinPulse = 2;
volatile unsigned long pulseCount;

// period is number of milliseconds to accumulate pulses
unsigned long period;
unsigned long msecTarget;
unsigned long loopCnt = 0;

void
Isr ()
{
    pulseCount++;
}

void setup () {
  Serial.begin (9600);
  pinMode (digitalPinPulse, INPUT);
  attachInterrupt (digitalPinToInterrupt (digitalPinPulse), Isr, RISING);

  period = 60000;  // milliseconds interval between readouts

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
        pulseCount = 0;
        loopCnt++;
        // Serial.println();
        Serial.print(loopCnt);
        Serial.print(", ");
        Serial.println(pulseOut);
    }
    // Serial.println(msec);  // keep-alive signal (!?)
    delay(10);
}
