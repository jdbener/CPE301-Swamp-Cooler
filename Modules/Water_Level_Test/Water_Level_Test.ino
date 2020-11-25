/*
 * Water_Level_test.ino
 * File which reads the water level.
 *
 * Results seam to range between 0 when I wipe the sensor dry
 * and 320 when it is completely submerged.
 * Then tend to be ~10 when not sumberged and not dry
 *
 * Modification History
 * Name          Date          Comment
 * Joshua Dahl   11/12/2020    Converted a polling scheme of reading from the ADC (lab 8)
 *                              into an interupt based scheme.
 */

// ADC Registers
volatile uint8_t* const admux = (uint8_t*) 0x7c;
volatile uint8_t* const adcsra = (uint8_t*) 0x7a;
volatile uint8_t* const adcsrb = (uint8_t*) 0x7b;
volatile uint16_t* const adcData = (uint16_t*) 0x78;


// ADC conversion complete interupt handler
ISR(ADC_vect){
  Serial.println(*adcData);

  // Start the next conversion
  *adcsra |= (1 << 6);
}

void setup() {
  // ADC setup
  adcInit();
  // Set the adc to be reading from channel 0
  setAdcChannel(0);

  // Set the baud rate
  Serial.begin(9600);

  // Enable interupts globally
  sei();
}

void loop() {
  // Read the value on the analog port and print it out.
  //Serial.println(adc_read(0));
}


// Sets the required initial settings for the adc
void adcInit(){
  // 7 = enable,
  // 5 = auto trigger = disable
  // 3 = interrupt = enable,
  // 0-2 = no prescaling = 0
  *adcsra = 0b10001000;

  // 6 = Comparator mux = disabled,
  // 3 = ADC mux overflow bit = 0
  // 0-2 = Auto trigger source = already disabled
  *adcsrb = 0;

  // 6-7 = Analog Reference = 0b01
  // 5 = Right Adjusted = 0
  // 0-4 = Cleared mux = 0
  *admux = 0b01000000;
}

// Disables the ADC
void adcDisable(){
  // Bit 7 = disable
  *adcsra = 0;
}

// Sets which channel the ADC is fed by
//  The results of this function won't be applied until the current conversion is finished.
void setAdcChannel(uint8_t channel){
  // Clear most of the MUX
  *admux &= 0b11100000;
  // Clear the MUX bit in adcsrb
  *adcsrb &= ~(1 << 3);

  // If the selected channel is in the second port
  if(channel >= 8){
    // Drop off the leading bit from the channel
    channel -= 8;

    // Set the MUX bit in adcsrb (Sets that we are using channels 8-16)
    *adcsrb |= (1 << 3);
  }

  // Choose which channel we are using (0-7 or 8-16 depending on results of the if)
  *admux |= channel;

  // Start the conversion
  *adcsra |= (1 << 6);
}
