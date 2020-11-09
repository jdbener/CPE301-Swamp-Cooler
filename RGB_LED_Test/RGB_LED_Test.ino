
/*
* RGB_LED_Test.ino
* Program which lights up the several leds of the RGB LED to create any of the primary colors.
* 
* MODIFICATION HISTORY
* Name          Date            Comment
* Joshua Dahl   11/9/2020       Created the setLEDColors function, driven by the LEDColors enum 
*                               NOTE: In order for correct color mixing to occure. A 1000ohm reistor
*                               must be connected to the green led and 330ohm reistors be connected
*                               to red and blue.
*/

// Enum which holds bitmasks for enabling each of the three LEDs in the RGB LED
enum LEDColors { RED = 0b100, GREEN = 0b010, BLUE = 0b001, OFF = 0 };


// Port B Registers
volatile uint8_t* const portb = (uint8_t*) 0x25;
volatile uint8_t* const ddrb = (uint8_t*) 0x24;


void setup() {
  // LED Plugged into PB.7, PB.6, and PB.5 (set them as output)
  *ddrb = 0b11100000;

  // Make the LED yellow
  setLEDColors(RED | GREEN); 
}


void loop() {
  
}


// NOTE: In order for correct color mixing to occure. A 1000ohm reistor must be connected to 
//  the green led and 330ohm resistors must be connected to red and green.
// Enables the specified colors in the RGB LED
void setLEDColors(int color){
  // Turn off all of the LEDs
  *portb &= 0b00011111;

  // TODO: Experiment with *portb |= (color << 5)
  
  // If the red bit is set, turn on the red LED
  if(color & RED) *portb |= 0b10000000;
  // If the green bit is set, turn on the green LED
  if(color & GREEN) *portb |= 0b01000000;
  // If the blue bit is set, turn on the blue LED
  if(color & BLUE) *portb |= 0b00100000;
}
