/*
* Temp_Humidity_Test.ino
* Program which writes out the temperature and humidity
*
* MODIFICATION HISTORY
* Name          Date            Comment
* www.elegoo.com10/25/2020       Distributed initial version
* Joshua Dahl   11/11/2020      Modified program to measure the temperature as
*                               soon as the sensor has a new reading.
*                               (Also changed units to fahrenheit)
*/

#include <dht_nonblocking.h>

#define DHT_SENSOR_TYPE DHT_TYPE_11
#define DHT_SENSOR_PIN 14
// Temp/Humidity sensor
DHT_nonblocking dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);


void setup() {
  // Set the baud rate
  Serial.begin(9600);
}

void loop() {
  // Poll for a measurement and proceed if the sensor has one to give us
  float temperature, humidity;
  if(dht_sensor.measure(&temperature, &humidity)) {
    // Convert the temperature from celsius to fahrenheit
    temperature = temperature * 9.0/5 + 32;

    // Print out the results
    Serial.print("T = ");
    Serial.print(temperature, 1);
    Serial.print(" F, H = ");
    Serial.print(humidity, 1);
    Serial.println("%");
  }
}
