//  Arduino code for Analog Devices 24-bit ADC AD7124
//  using library https://github.com/epsilonrt/ad7124

//  reads out temperature from two 100-ohm RTD sensors in 3-wire mode
//  driven by the 2 current sources in the AD7124

//  RTD1 on AIN2/3 with current from AIN0/1
//  RTD2 on AIN4/5 with current from AIN6/7
//  both RTD low sides connected to REFIN1+
//  5.00k resistor from REFIN1+ to REFIN1-
//  REFIN1- through 250 ohm resistor to AGND (AVSS)
//  see: analog.com cn0383.pdf Figure 21 (multiple 3-wire RTD config)

//  AD7124 chip is connected on the Arduino MOSI, MISO, SCK and /SS pins (pin 10)
//  -- J.Beale 21-Aug-2022

#include <ad7124.h>

using namespace Ad7124;

const int ssPin = 10;  // AD7124 chip select pin
Ad7124Chip adc;

// -----------------------------------------------------------------------------
void setup() {

  Serial.begin (38400);
  Serial.println ("AD7124 RTD measurement");
  adc.begin (ssPin);  // initialize AD7124 with pin /CS

  // adc.setConfigFilter (0, Sinc4Filter, 384); // 50 Hz reading, 50/4 output rate
  adc.setConfigFilter (0, Sinc4Filter, 2047);  // 2047 = 2.34 Hz output rate
  adc.setAdcControl (SingleConvMode, FullPower, true);
  adc.setConfig (0, RefIn1, Pga32, false); // Config 0, 'false' => unipolar, straight-binary output  
  // adc.setCurrentSource (ch, ch, CurrentOff);  // turn off bias currents
}


// convert RTD resistance in ohms to temperature in deg.C
double RtoC(double Rt) {

  const double A = 3.9083E-3;    // Callendar-Van Dusen coefficient A
  const double B = -5.775E-7;    // Callendar-Van Dusen coefficient B
  const double Ro = 100.0;       // RTD resistance at 0 C
  double degC = (sqrt(A*A - 4*B*(1-(Rt/Ro))) - A) / (2*B);  // valid only for T > 0 C
  return (degC);
}

// -----------------------------------------------------------------------------

void loop() {
  
  const double gain_PGA = 32.0;  // gain from onboard PGA in ADC
  const double Rref = 5000.0;    // reference resistor (gets 2x RTD current) 
  const double code_max = (1L<<24) - 1;    // unipolar max ADC code is 2^24 - 1 

// --------------------------------------------------------------------------
  adc.setChannel (0, 0, AIN2Input, AIN3Input);  // chan0, config0, input  AIN2(+)/AIN3(-)
  adc.setCurrentSource (0, 0, Current250uA); // source, output channel, current
  adc.setCurrentSource (1, 1, Current250uA); // source, output channel, current

  long code_adc = adc.read (0);  // single-conversion ADC reading
  double Rt = (code_adc / code_max) * (2*Rref / gain_PGA); // 3-wire setup: 2x RTD current flows through Rref
  double degC1 = RtoC(Rt);  // convert resistance to temperature

// --------------------------------------------------------------------------
  adc.setChannel (0, 0, AIN4Input, AIN5Input);  // chan0, config0, input  (+)/(-)
  adc.setCurrentSource (0, 6, Current250uA); // source, output channel, current
  adc.setCurrentSource (1, 7, Current250uA); // source, output channel, current

  code_adc = adc.read (0);  // single-conversion ADC reading
  Rt = (code_adc / code_max) * (2*Rref / gain_PGA); // 3-wire setup: 2x RTD current flows through Rref
  double degC2 = RtoC(Rt);  // convert resistance to temperature

  Serial.print(degC1,6);   // calculated RTD temperature in degrees C
  Serial.print(", ");  
  Serial.println(degC2,6);   // calculated RTD temperature in degrees C
}
