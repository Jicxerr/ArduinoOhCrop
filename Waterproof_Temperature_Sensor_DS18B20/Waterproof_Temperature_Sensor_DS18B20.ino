#include <OneWire.h>
#include <DallasTemperature.h>

#define WPTempSensor 12

OneWire oneWire(WPTempSensor);
DallasTemperature wptsensor(&oneWire);

float tempC; // temperature in Celsius
float tempF; // temperature in Fahrenheit

void setup() {
  Serial.begin(115200); // initialize serial
  wptsensor.begin();    // initialize the DS18B20 sensor
}

void loop() {
  wptsensor.requestTemperatures();
  tempC = wptsensor.getTempCByIndex(0);  // read temperature in °C
  tempF = tempC * 9 / 5 + 32; // convert °C to °F
  Serial.print("Temperature: ");
  Serial.print(tempC);    // print the temperature in °C
  Serial.print("°C");
  Serial.print("  ~  ");  // separator between °C and °F
  Serial.print(tempF);    // print the temperature in °F
  Serial.println("°F");
  Serial.println();
  delay(500);
}