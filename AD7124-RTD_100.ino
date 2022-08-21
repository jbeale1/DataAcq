//  Arduino code for Analog Devices 24-bit ADC AD7124
//  using library https://github.com/epsilonrt/ad7124

//  reads out temperature of 100-ohm RTD sensor in 3-wire mode
//  using the 2 current sources internal to the AD7124
//  AD7124 chip is connected on the Arduino MOSI, MISO, SCK and /SS pins (pin 10)
//  -- J.Beale 20-Aug-2022

#include <ad7124.h>

using namespace Ad7124;

const int ssPin = 10;  // AD7124 chip select pin
Ad7124Chip adc;

// -----------------------------------------------------------------------------
void setup() {

  Serial.begin (38400);
  Serial.println ("AD7124 RTD measurement");
  adc.begin (ssPin);  // initialize AD7124 with pin /CS

// -----------------------------------------------------

  // adc.setConfigFilter (0, Sinc4Filter, 384); // 50 Hz reading, 50/4 output rate
  adc.setConfigFilter (0, Sinc4Filter, 2047);  // 2047 = 2.34 Hz output rate

    // Setting channel 0 with config 0 using pins AIN2(+)/AIN3(-)
  adc.setChannel (0, 0, AIN2Input, AIN3Input);

  // adc.setAdcControl (StandbyMode, FullPower, true);
  adc.setAdcControl (SingleConvMode, FullPower, true);

  // Setting the configuration 0 for measuring
  adc.setConfig (0, RefIn1, Pga32, false); // 'false' => unipolar, straight-binary output
  
  // Program the excitation currents and output the currents on the AIN0/1
  adc.setCurrentSource (0, 0, Current250uA); // source, output channel, current
  adc.setCurrentSource (1, 1, Current250uA); // source, output channel, current
  // adc.setCurrentSource (ch, ch, CurrentOff);  // turn off bias currents
}

// -----------------------------------------------------------------------------

void loop() {
  
  long code_adc;                  // raw ADC output code
  const double gain_PGA = 32.0;  // gain from onboard PGA in ADC
  const double Rref = 5000.0;    // reference resistor (gets 2x RTD current) 
  const double code_max = (1L<<24) - 1;    // unipolar max ADC code is 2^24 - 1 
  const double A = 3.9083E-3;    // Callendar-Van Dusen coefficient A
  const double B = -5.775E-7;    // Callendar-Van Dusen coefficient B
  const double Ro = 100.0;       // RTD resistance at 0 C
  double Rt;                     // calculated RTD resistance
  double degC;                   // temperature in degrees C (valid only for T > 0)

  code_adc = adc.read (0);  // single-conversion ADC reading
  Rt = (code_adc / code_max) * (2*Rref / gain_PGA); // 3-wire setup: 2x RTD current flows through Rref
  degC = (sqrt(A*A - 4*B*(1-(Rt/Ro))) - A) / (2*B);  // valid only for T>0 C
  
  Serial.println(degC,6);   // calculated RTD temperature in degrees C
}
/* ========================================================================== */
