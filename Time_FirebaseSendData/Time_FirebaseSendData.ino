#include <WiFi.h>
#include <time.h>

const char *ssid = "HUAWEI-2.4G-8NqA";
const char *password = "p69CE656";

struct tm timeinfo;
int hour, DayToday;
int mins, secs;
std::string Date;

void firebaseSendData() {
    // Your function to send data to Firebase
    // Replace this with your actual implementation
    Serial.println("Sending data to Firebase");
}

unsigned long lastExecutionTime = 0;
const unsigned long interval = 3600000;  // 1 hour in milliseconds
bool dataSentThisHour = false;

void setup() {
    Serial.begin(115200);
    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");

    time_t now;
    while ((now = time(nullptr)) < 1577836800) {  // January 1, 2020
      Serial.println("Waiting for time to be set...");
      delay(2000);
    }

}

void loop() {
    
    // Your other code in the loop
    if (!getLocalTime(&timeinfo)) {
      Serial.println("Failed to obtain time");
      return;
    }

        // Print the current time
      Serial.printf("Current time: %04d-%02d-%02d %02d:%02d:%02d\n",
                timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                timeinfo.tm_hour+8, timeinfo.tm_min, timeinfo.tm_sec);

    lastExecutionTime = millis();  // Update the last execution time
        // Format the date
    Date = std::to_string(timeinfo.tm_mday) + "-" + std::to_string(timeinfo.tm_mon + 1) + "-" + std::to_string(timeinfo.tm_year + 1900);
    // Get the hour
    hour = timeinfo.tm_hour;
    mins = timeinfo.tm_min;
    secs = timeinfo.tm_sec;

    // Check if an hour has passed since the last execution
    // if (millis() - lastExecutionTime >= interval) {
    //     lastExecutionTime = millis();  // Update the last execution time
    //     dataSentThisHour = false;      // Reset the flag for the new hour
    // }
        
    // Check the hour and send data if conditions are met
    //if(!dataSentThisHour){
      if ((hour == 7 || hour == 8 || hour == 9 || hour == 10 || hour == 11 || hour == 12 || hour == 13 || hour == 14 || hour == 15 || hour == 16 || hour == 17)&& mins == 59 && secs >= 50) {
          firebaseSendData();
          //dataSentThisHour = true;
      }
    //}
    // Your other code in the loop

    delay(1000);
}
