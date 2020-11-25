/*
* Realtime_Clock_Test.ino
* Program which writes out the current date and time
* 
* MODIFICATION HISTORY
* Name           Date            Comment
* www.elegoo.com 10/24/2018      Distributed initial version
* Joshua Dahl    11/12/2020      Added function which prints the current timestamp
*                                 to the Serial monitor.
*/
#include <Wire.h>
#include <DS3231.h>

// Uncomment to reset the clock time
//#define RESET_CLOCK_TIME
DS3231 clock;

void setup()
{
  Serial.begin(9600);

  Serial.println("Initialize RTC module");
  // Initialize DS3231
  clock.begin();
  
  // Send sketch compiling time to Arduino.
  //  Should be disabled after first run.
  //  If this continues to occure the date and time will be
  //    reset on each run of the program.
#if defined(RESET_CLOCK_TIME)
  Serial.println("Resynced clock memory.");
  clock.setDateTime(__DATE__, __TIME__);
#endif
}

// Function which prints the timestamp of when an arbitrary event occured
void printTimestamp(const char* event = "X"){
  RTCDateTime dt = clock.getDateTime();

  Serial.print(event);     Serial.print(" occured at: ");
  Serial.print(dt.year);   Serial.print("-");
  Serial.print(dt.month);  Serial.print("-");
  Serial.print(dt.day);    Serial.print(" ");
  Serial.print(dt.hour);   Serial.print(":");
  Serial.print(dt.minute); Serial.print(":");
  Serial.print(dt.second); Serial.println("");
}

void loop()
{
  printTimestamp();

  delay(1000);
}
