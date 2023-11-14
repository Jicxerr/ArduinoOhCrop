#include <DHT.h>


DHT dht(15, DHT11);
void setup() {
  dht.begin();
  delay(2000);
  Serial.begin(115200);

}

void loop() {
  float temp = dht.readTemperature();
  float humidity = dht.readHumidity();
  Serial.print("Temp: ");
  Serial.print(temp);
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print("%");
  Serial.println();
  delay(2000);
}
