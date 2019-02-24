
/*
 * This is a half bridge PWM driver for Ardiono Due.
 * It produces H and L pwm signals on Pins 34, 35 to drive a Mostfet driver like the ir2110.
 * it a minimum dead time of 1us and can be adjusted over the serial line.
 */


#define MASTER_CLOCK 84000000
uint32_t clock_a = 42000000; // Sampling frequency in Hz
uint32_t steps = 600;
uint32_t frequency = 42000000/(2*steps);
uint32_t clockFrequency = 42000000;
uint32_t dutyCycle = 100;

//#define HBRIDGE 1


// if the base cycles is 2 the drvier is configured as a H bridge, if 1 its configured as a 
// high or low side driver only.
#ifdef HBRIDGE
#define BASE_CYCLES 2
#else
#define BASE_CYCLES 1
#endif

void setup() 
{
  SetPin(34); // PWML0

#ifdef HBRIDGE
  SetPin(35); // PWMH0
#endif
  
  pmc_enable_periph_clk(PWM_INTERFACE_ID); // Turn on PWM clock
  PWMC_ConfigureClocks(clock_a, 0, MASTER_CLOCK); // clock_b = 0
#ifdef HBRIDGE
  PWMC_ConfigureChannelExt(PWM,
                           0, // Channel: 0          
                           PWM_CMR_CPRE_CLKA, // Prescaler: use CLOCK_A
                           PWM_CMR_CALG, // Alignment: period is center aligned
                           0, // Polarity: output waveform starts at a low level
                           PWM_CMR_CES, // Counter event: occurs at the end and middle of the period
                           PWM_CMR_DTE, // Dead time generator is enabled
                           0, // Dead time PWMH output is not inverted    
                           0);  // Dead time PWML output is not inverted
  PWMC_SetPeriod(PWM, 0, steps); // Channel: 0, Period: 1/(1200/42 Mhz) = ~35 kHz
  PWMC_SetDutyCycle(PWM, 0, steps/BASE_CYCLES); // Channel: 0, Duty cycle: 50 %
  PWMC_SetDeadTime(PWM, 0, 42, 42); // Channel: 0, Rising and falling edge dead time: 42/42 Mhz = 1 us
#else
  PWMC_ConfigureChannel(PWM,
                           0, // Channel: 0          
                           PWM_CMR_CPRE_CLKA, // Prescaler: use CLOCK_A
                           0, // Alignment: period start aligned
                           0 // Polarity: output waveform starts at a low level
                           );
  PWMC_SetPeriod(PWM, 0, steps); // Channel: 0, Period: 1/(1200/42 Mhz) = ~35 kHz
  PWMC_SetDutyCycle(PWM, 0, steps); // Channel: 0, Duty cycle: 100 %
//  PWMC_SetDeadTime(PWM, 0, 42, 42); // Channel: 0, Rising and falling edge dead time: 42/42 Mhz = 1 us
#endif
  PWMC_EnableChannel(PWM, 0); // Channel: 0
  Serial.begin(115200);
  Serial.println("Setup with 42Mzh clock, 35KHz Pwm, 600 steps");
  Serial.println("d35 sets duty cycle to 35%, f35000 sets pwm to 35kHz. s600 sets steps to 600");
  Serial.println("Max clock is 42Mhz, lowering the steps gives a higher max pwm frequency");
}

void loop()
{
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    String data = input.substring(1);
    switch(input.charAt(0)) {
      case 'd':
      setDutyCycle(data.toInt());
      break;
      case 'f':
      setFrequency(data.toInt());
      break;
      case 's':
      setSteps(data.toInt());
      break;
    }
  }
}

void setFrequency(uint32_t newFrequency) {
  frequency = newFrequency;
  clockFrequency = frequency*(steps*BASE_CYCLES);
  if (clockFrequency > clock_a) {
    clockFrequency = clock_a;
  }
  PWMC_ConfigureClocks(clockFrequency, 0, MASTER_CLOCK); // clock_b = 0
  Serial.print(clockFrequency);
  Serial.print(" Hz Clock ");
  Serial.print((1.0*clockFrequency)/(steps*BASE_CYCLES));
  Serial.println(" Hz Pwm ");
}
void setSteps(uint32_t newSteps) {
#ifdef HBRIDGE
  steps = newSteps;
  setFrequency(frequency);
  PWMC_SetPeriod(PWM, 0, steps); // Channel: 0, Period: 1/(1200/42 Mhz) = ~35 kHz
  PWMC_SetDutyCycle(PWM, 0, steps/BASE_CYCLES); // Channel: 0, Duty cycle: 50 %
  Serial.print(steps);
  Serial.print(" Steps ");
  Serial.print((1.0*clock_a)/(steps*BASE_CYCLES));
  Serial.println(" Hz max frequency ");
#else
  steps = newSteps;
  setFrequency(frequency);
  PWMC_SetPeriod(PWM, 0, steps); // Channel: 0, Period: 1/(1200/42 Mhz) = ~35 kHz
  setDutyCycle(dutyCycle);
  Serial.print(steps);
  Serial.print(" Steps ");
  Serial.print((1.0*clock_a)/(steps*BASE_CYCLES));
  Serial.println(" Hz max frequency ");
#endif

}
void setDutyCycle(uint32_t newDutyCycle) {
#ifdef HBRIDGE
  dutyCycle = newDutyCycle;
    // make the dead time 1us (42/42MHz) minimum.
    int deadTime = 42+(((steps-42)*(100-dutyCycle))/100);
    PWMC_SetDeadTime(PWM, 0, deadTime, deadTime); // Channel: 0, Rising and falling edge dead time: 42/42 Mhz = 1 us
    Serial.print(dutyCycle);
    Serial.print("% Dutycycle gives ");
    Serial.print((1.0*deadTime)/(clockFrequency/1000000.0));
    Serial.println("us Deadtime");
#else
  dutyCycle = newDutyCycle;
  int dutyCycleSteps = (steps*dutyCycle)/100;
  PWMC_SetDutyCycle(PWM, 0, dutyCycleSteps); // Channel: 0
  Serial.print(dutyCycle);
  Serial.print("% Dutycycle gives ");
  Serial.print(dutyCycleSteps);
  Serial.println(" steps");
#endif

}

void SetPin(uint8_t pin)
{
  PIO_Configure(g_APinDescription[pin].pPort, // Port
                PIO_PERIPH_B, // Peripheral
                g_APinDescription[pin].ulPin,
                g_APinDescription[pin].ulPinConfiguration);

}
