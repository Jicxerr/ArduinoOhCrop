#include <WiFi.h>
#include <time.h>

const char *ssid = "HUAWEI-2.4G-8NqA";
const char *password = "p69CE656";

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Configure time with NTP
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  // Wait for time to be set
  time_t now;
  while ((now = time(nullptr)) < 1577836800) {  // January 1, 2020
    Serial.println("Waiting for time to be set...");
    delay(2000);
  }

  // Print current time
  printLocalTime();
}

void loop() {
  // Your code here
  delay(1000);

  printLocalTime();
}

void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }

  Serial.printf("Current time: %04d-%02d-%02d %02d:%02d:%02d\n",
                timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                timeinfo.tm_hour-4, timeinfo.tm_min, timeinfo.tm_sec);
}
