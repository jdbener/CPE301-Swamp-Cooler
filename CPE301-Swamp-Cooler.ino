
/*
 * CPE301-Swamp-Cooler.ino
 * Main file controlling the major execution of the project.
 *
 * Modification History
 * Name          Date          Comment
 * www.elegoo.com 10/24/2018   Distributed initial version of module code
 * Joshua Dahl   10/22/2020    Created file with setup and loop
 * Joshua Dahl   11/9/2020     Created the setLEDColors function, driven by
 *                              the LEDColors enum.
 *                              NOTE: In order for correct color mixing to
 *                              occur. A 1000ohm resistor must be connected to
 *                              the green LED and 220ohm resistors be connected
 *                              to red and blue.
 * Joshua Dahl   11/10/2020    Created initial version, of interrupt button
 *                              ran into issues with debouncing not working
 *                              properly so added a timing mechanism to fix it.
 *                        Assumes button is connected to PE.4 = Pin PWM 2 = INT4
 * Joshua Dahl   11/11/2020    Modified temp/humidity sample program to measure
 *                              the temperature as soon as the sensor has a new
 *                              reading. (Also changed units to fahrenheit)
 * Joshua Dahl   11/12/2020    Added function which prints the current timestamp
 *                              to the Serial monitor.
 * Joshua Dahl   11/12/2020    Converted a polling scheme of reading from the
 *                              ADC (lab 8) into an interrupt based scheme.
 *                              Results seam to range between 0 when I wipe the
 *                              sensor dry and 320 when it is completely
 *                              submerged. Then tend to be ~10 when not
 *                              submerged and not dry.
 *                     Assumes Water Level Sensor is connected to analog port 0.
 * Joshua Dahl   11/17/2020    Modified LCD example code to print out the
 *                              temperature and humidity.
 *                              LCD RS pin to digital pin 37
 *                              LCD Enable pin to digital pin 35
 *                              LCD D4 pin to digital pin 33
 *                              LCD D5 pin to digital pin 31
 *                              LCD D6 pin to digital pin 29
 *                              LCD D7 pin to digital pin 27
 * Joshua Dahl   11/20/2020    Added stepper motor (after trying to get a DC
 *                              motor to work for a few days)
 * Joshua Dahl   11/24/2020    Updated the setLEDColors function to not relie
 *                              on an if block.
 * Joshua Dahl   11/24/2020    Merged module code together, added code for
 *                              switching between a more complex state layout
 * Joshua Dahl   11/25/2020    Added code for printing relevant information on
 *                               the LCD
 */
#include <Wire.h>                // i2c
#include <DS3231.h>              // Real Time Clock
#include <dht_nonblocking.h>     // Temperature/Humidity Sensor
#include <Stepper.h>             // Stepper Motor
#include <LiquidCrystal.h>       // LCD


// Define a byte
typedef uint8_t byte;

// Enum which holds bitmasks for enabling each of the three LEDs in the RGB LED
enum LEDColors { RED = 0b100, GREEN = 0b010, BLUE = 0b001, OFF = 0 };

// Port H Registers
volatile byte* const porth = (byte*) 0x102;
volatile byte* const ddrh = (byte*) 0x101;

// Port E Registers
volatile byte* const porte = (byte*) 0x2E;
volatile byte* const ddre = (byte*) 0x2D;
volatile byte* const pine = (byte*) 0x2C;

// Timer 1 Registers
volatile byte* const timsk1 = (byte*) 0x6F;
volatile byte* const tccr1a = (byte*) 0x80;
volatile byte* const tccr1b = (byte*) 0x81;
volatile uint16_t* const tcnt1 = (uint16_t*) 0x84;

// ADC Registers
volatile byte* const admux = (byte*) 0x7c;
volatile byte* const adcsra = (byte*) 0x7a;
volatile byte* const adcsrb = (byte*) 0x7b;
volatile uint16_t* const adcData = (uint16_t*) 0x78;

// Interrupt Registers
volatile byte* const eimsk = (byte*) 0x3D;
volatile byte* const eicrb = (byte*) 0x6A;



// Variable tracking the current state of the program
enum State {Disabled, Idle, Running, Error} state;

// Uncomment to reset the clock time
//#define RESET_CLOCK_TIME
// Variable representing our connection to the realtime clock
DS3231 clock;

#define DHT_SENSOR_TYPE DHT_TYPE_11
#define DHT_SENSOR_PIN 14
// Temp/Humidity sensor
DHT_nonblocking dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);

#define stepsPerRevolution 2048  // change this to fit the number of steps per revolution
#define rolePerMinute 17         // Adjustable range of 28BYJ-48 stepper is 0~17 rpm
// initialize the stepper library on pins 9 through 12:
Stepper stepperMotor(stepsPerRevolution, 9, 11, 10, 12);

// initialize the lcd with the numbers of the interface pins
LiquidCrystal lcd(37, 35, 33, 31, 29, 27);

/*------------------------------------------------------------------------------
    Constants
------------------------------------------------------------------------------*/

// The observed maximum of the water level sensor
#define MAX_WATER_LEVEL 320
// The water level the system must be above to not be in error
#define WATER_LEVEL_THRESHOLD 50
// The fahrenheit temperature which must be exceeded to enable the fan
#define TEMPERATURE_THRESHOLD 80

/*------------------------------------------------------------------------------
    Prototypes
------------------------------------------------------------------------------*/

void printTimestamp(const char* event = nullptr);

/*------------------------------------------------------------------------------
    Interrupt Handlers
------------------------------------------------------------------------------*/

// INT4 interrupt handler
// Starts Timer1 when the button is pressed
ISR(INT4_vect){
    // Start the timer (prescaler 1/256) set to run for .25 seconds
    *tcnt1 = 34286;
    *tccr1b = (1 << 2);
}

// Timer1 overflowing interrupt handler
// Debounces the input by checking that the button is still pressed
//  then switches the state of the program (disabled -> idle, other -> disabled)
ISR(TIMER1_OVF_vect){
    // Stop the timer
    *tccr1b &= 0b11111000;

    // Check that the pin is still low
    if(!(*pine & (1 << 4))){
        // Handle state change (disabled -> idle, other -> disabled)
        if(state == Disabled) changeState(Idle);
        else changeState(Disabled);
    }
}

// ADC conversion complete interrupt handler
// Switches the state to Error if the water level is too low
//  and resets to the Idle state if the water level returns to acceptable limits
ISR(ADC_vect){
    // If we are in a state which cares about water level
    if(state == Running || state == Idle){
        // If the water level is too low
        if(*adcData <= WATER_LEVEL_THRESHOLD)
            // Jump to the error state
            changeState(Error);
    // If we are in the error state
    } else if (state == Error)
        // If the water level has returned to an acceptable range
        if(*adcData > WATER_LEVEL_THRESHOLD)
            // Return to the idle state
            // (temperature polling may then pull it to the running state)
            changeState(Idle);

    // Start the next conversion
    *adcsra |= (1 << 6);
}

/*------------------------------------------------------------------------------
    Core Functions
------------------------------------------------------------------------------*/

void setup(){
    // The program always begins in the disabled state
    state = Disabled;

    // Set Serial baud rate
    Serial.begin(9600);

    // Enable the RGB LED
    setupRGBLED();

    // Enable the button interrupt and timer
    setupDisableButton();

    // Initialize DS3231 (Real Time Clock)
    clock.begin();
//     // Send sketch compiling time to Arduino.
//     //  Should be disabled after the first run.
//     //  If this continues to occur the date and time will be
//     //    reset on each run of the program.
// #if defined(RESET_CLOCK_TIME)
//     Serial.println("Resynced clock memory.");
//     clock.setDateTime(__DATE__, __TIME__);
// #endif

    // ADC setup
    adcInit();
    // Set the adc to be reading from channel 0
    setAdcChannel(0);

    // Setup Stepper Motor
    stepperMotor.setSpeed(rolePerMinute);

    // Setup the LCD's number of columns and rows:
    lcd.begin(16, 2);
    // Set LCD state display
    lcd.print("Disabled");

    // Enable interrupts globally
    sei();
}

void loop(){
    // Variables representing the measured temperature and humidity
    static float temperature, humidity;

    // Only poll temperature and humidity in the idle and running states
    if(state == Idle || state == Running){
        // Poll for a measurement and proceed if the sensor has one to give us
        if(dht_sensor.measure(&temperature, &humidity)) {
            // Convert the temperature from celsius to fahrenheit
            temperature = temperature * 9.0/5 + 32;

            // Print out the results
            // Set the cursor to column 0, line 1
            // (NOTE: line 1 is the second row, since counting begins with 0):
            lcd.setCursor(0, 1);
            lcd.print(temperature, 1);
            // Degrees Fahrenheit
            lcd.print((char) 0b11011111); lcd.print("F");
            lcd.print(" - ");
            lcd.print(humidity, 1); lcd.print("%RH");

            // Polled transitions (idle -> running and running -> idle) based
            //  on temperature.
            if(state == Idle && temperature > TEMPERATURE_THRESHOLD)
                changeState(Running);
            else if(state == Running && temperature < TEMPERATURE_THRESHOLD)
                changeState(Idle);
        }
    }

    // Only run the stepper motor in the running state
    if(state == Running) stepperMotor.step(rolePerMinute);

    // Print out the current water level percentage while in the error state
    if(state == Error){
        // Set the cursor to column 0, line 1
        // (NOTE: line 1 is the second row, since counting begins with 0):
        lcd.setCursor(0, 1);
        lcd.print("Water Level: ");
        lcd.print(float(*adcData) / MAX_WATER_LEVEL, 1);
        lcd.print("%");
    }
}

// Function which transitions from one state to another
void changeState(State newState){
    // No Interrupts while changing state
    cli();

    // Print the timestamp of the state transition
    // TODO: Print information about which state from and to
    printTimestamp("bob");

    // Handle any transition specific details between specific states
    // Disabled -> Any Other
    if(state == Disabled){
        // Make sure everything needed for operation is turned on

        // ADC setup
        adcInit();
        // Set the adc to be reading from channel 0
        setAdcChannel(0);
    }

    // Handle anything which needs to occur when we enter a state
    switch(newState){
    case Disabled:
        // TODO: turn off stuff
        adcDisable();

        // Make the LED yellow
        setLEDColors(RED + GREEN);
        // Set LCD state display
        lcd.print("Disabled");
        break;
    case Idle:

        // Make the LED green
        setLEDColors(GREEN);
        // Set LCD state display
        lcd.print("Idling");
        break;
    case Running:

        // Make the LED blue
        setLEDColors(BLUE);
        // Set LCD state display
        lcd.print("Running Fan");
        break;
    case Error:

        // Make the LED red
        setLEDColors(RED);
        // Set LCD state display
        lcd.print("Error (Refill Water)");
        break;
    }

    // Mark that the state has changed
    state = newState;
    // Enable interrupts globally
    sei();
}

/*------------------------------------------------------------------------------
    Setup Functions
------------------------------------------------------------------------------*/

void setupRGBLED(){
    // LED Plugged into PH.5, PH.4, and PH.3 (set them as output)
    *ddrh = 0b00111000;

    // Make the LED yellow
    setLEDColors(RED + GREEN);
}

void setupDisableButton(){
    // No compare match or waveform generation
    *tccr1a = 0;
    // No input filtering/waveform generation; timer stopped
    *tccr1b = 0;
    // Enable the overflow interrupt
    *timsk1 = 1;

    // Enable pullup resistor on PE.4 and make sure the port is in input mode
    *ddre = 0;
    *porte = (1 << 4);

    // Enable the interrupt on PE.4 (INT4)
    *eimsk = (1 << 4);
    // Set INT4 to be triggered on the falling edge (button is pressed)
    *eicrb = 0b00000010;
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

/*------------------------------------------------------------------------------
    Module Functions
------------------------------------------------------------------------------*/

// NOTE: In order for correct color mixing to occur. A 1000ohm resistor must be connected to
//  the green led and 220ohm resistors must be connected to red and green.
// Enables the specified colors in the RGB LED
void setLEDColors(int color){
    // Turn off all of the LEDs
    *porth &= 0b11000111;

    // Turn on the LEDs specified in color
    *porth |= (color << 3);
}

// Function which prints the timestamp of when an arbitrary event occured
void printTimestamp(const char* event){
    RTCDateTime dt = clock.getDateTime();

    if(event){ Serial.print(event);     Serial.print(" occurred at: "); }
    Serial.print(dt.year);   Serial.print("-");
    Serial.print(dt.month);  Serial.print("-");
    Serial.print(dt.day);    Serial.print(" ");
    Serial.print(dt.hour);   Serial.print(":");
    Serial.print(dt.minute); Serial.print(":");
    Serial.print(dt.second); Serial.println("");
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
