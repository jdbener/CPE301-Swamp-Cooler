 /*
 * Button_Interupt_Test.ino
 * File which tests using a button's interupt to change state
 * The button must be held for a quarter second for its input to register
 *
 * Assumes button is connected to PE.4 = Pin PWM 2 = INT4
 *
 * Modification History
 * Name          Date          Comment
 * Joshua Dahl   11/10/2020    Created initaial version, ran into issues with
 *                              debouncing not working properly so added
 *                              a timing mechanism to fix it
 */


// Port B Registers
volatile uint8_t* const portb = (uint8_t*) 0x25;
volatile uint8_t* const ddrb = (uint8_t*) 0x24;

// Port E Registers
volatile uint8_t* const porte = (uint8_t*) 0x2E;
volatile uint8_t* const ddre = (uint8_t*) 0x2D;
volatile uint8_t* const pine = (uint8_t*) 0x2C;

// Timer 1 Registers
volatile uint8_t* const timsk1 = (uint8_t*) 0x6F;
volatile uint8_t* const tccr1a = (uint8_t*) 0x80;
volatile uint8_t* const tccr1b = (uint8_t*) 0x81;
volatile uint16_t* const tcnt1 = (uint16_t*) 0x84;

// Interupt Registers
volatile uint8_t* const eimsk = (uint8_t*) 0x3D;
volatile uint8_t* const eicrb = (uint8_t*) 0x6A;



enum State {Disabled, Running} state;

// INT4 interupt handler
// Starts Timer1 when the button is pressed
ISR(INT4_vect){
  // Start the timer (prescaler 1/256) set to run for .25 seconds
  //*tcnt1 = 34286;
  *tcnt1 = 17143;
  *tccr1b = (1 << 2);
}

// Timer1 reaching its count interrupt handler
// Debounces the input by checking that the button is still pressed
//  then switches the state of the program
ISR(TIMER1_OVF_vect){
  // Stop the timer
  *tccr1b &= 0b11111000;

  //Check that the pin is still low
  if(!(*pine & (1 << 4))){
    // Swap the state to running if currently disabled.
    if(state == Disabled) state = Running;
    // If the state is anything other than disabled, swap the state to disabled
    else state = Disabled;
  }
}

void setup(){
  // Make sure the state is set to disabled at the start of the program
  state = Disabled;

  // No compare match or waveform generation
  *tccr1a = 0;
  // No input filtering/waveform generation; timer stopped
  *tccr1b = 0;
  // Enable the overflow interupt
  *timsk1 = 1;

  // Set PB.7 (internal LED) to output mode
  *ddrb = (1 << 7);

  // Enable pullup resistor on PE.4 and make sure the port is in input mode
  *ddre = 0;
  *porte = (1 << 4);

  // Enable the interupt on PE.4 (INT4)
  *eimsk = (1 << 4);
  // Set INT4 to be triggered on the falling edge (button is pressed)
  *eicrb = 0b00000010;
  // Enable interupts globably
  sei();
}


void loop(){
  // Turn on the internal LED while running and turn it off while disabled
  if(state == Disabled) *portb &= ~(1 << 7);
  else *portb |= (1 << 7);
}
