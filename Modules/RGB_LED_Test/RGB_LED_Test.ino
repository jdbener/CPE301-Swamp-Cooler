
/*
* RGB_LED_Test.ino
* Program which lights up the several leds of the RGB LED to create any of the primary colors.
* 
* MODIFICATION HISTORY
* Name          Date            Comment
* Joshua Dahl   11/9/2020       Created the setLEDColors function, driven by the LEDColors enum 
*                               NOTE: In order for correct color mixing to occure. A 1000ohm reistor
*                               must be connected to the green led and 220ohm reistors be connected
*                               to red and blue.
* Joshua Dahl   11/24/2020      Updated the setLEDColors function to not relie on an if block.       
*/

// Enum which holds bitmasks for enabling each of the three LEDs in the RGB LED
enum LEDColors { RED = 0b100, GREEN = 0b010, BLUE = 0b001, OFF = 0 };


// Port H Registers
volatile uint8_t* const porth = (uint8_t*) 0x102;
volatile uint8_t* const ddrh = (uint8_t*) 0x101;


void setup() {
  // LED Plugged into PH.5, PH.4, and PH.3 (set them as output)
  *ddrh = 0b00111000;

  // Make the LED yellow
  setLEDColors(RED + GREEN); 
}


void loop() {
  
}


// NOTE: In order for correct color mixing to occure. A 1000ohm reistor must be connected to 
//  the green led and 220ohm resistors must be connected to red and green.
// Enables the specified colors in the RGB LED
void setLEDColors(int color){
  // Turn off all of the LEDs
  *porth &= 0b11000111;

  *porth |= (color << 3);
}
