#include <Arduino.h>
#include <ArduinoJson.h>
#include <HardwareSerial.h>

#if defined(ESP32)
#include <WiFi.h>
#include <WiFiMulti.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

#include <time.h>                 //Time
#include <Firebase_ESP_Client.h>  //Firebase
#include "addons/TokenHelper.h"   //Firebase
#include "addons/RTDBHelper.h"    //Firebase

//#define WIFI_SSID "HUAWEI-2.4G-8NqA"
//#define WIFI_PASSWORD "p69CE656"
#define API_KEY "AIzaSyDrCAGQ8tClxtwseyOsFAbkMwWz_2MTUdI"
#define DATABASE_URL "https://ohcrop-65556-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define FIREBASE_PROJECT_ID "ohcrop-65556"

#define USER_EMAIL "jay@gmail.com"
#define USER_PASSWORD "123456"

//Firebase varaibles
FirebaseData fbdo, fbdo_s1, fbdo_s2;
FirebaseAuth auth;
FirebaseConfig config;

// WIFI **********************************************************
WiFiMulti wifiMulti;
// WiFi connect timeout per AP. Increase when connecting takes longer.
const uint32_t connectTimeoutMs = 10000;

// TIME **********************************************************
time_t now;


// DATA/TIME //reportsData**********************************************************
struct tm timeinfo;
int hour, DayToday, Month, Year;
int mins, secs;
std::string Date;

bool reportSentThisHour = false;


// Setting **********************************************************
bool sdevice = true;
String user_uid;   //UserID
String wifi_ssid;  // wifi_ssid
String esp32_ip;   // esp32_ip address

//**********************************************************
unsigned long sendDataPrevMillis = 0;
unsigned long sendDataPrevMillis1 = 0;
unsigned long sendDataPrevMillis2 = 0;
bool signupOK = false;
int ldrData = 0;
float voltage = 0.0;
int pwmValue = 0;
bool ledStatus = false;
int count = 0;

// SENSOR UART Data VARIABLES VALUES From Arduino
float watertemp;
float humidity;
float airtemp;
int waterlevel;
float ph;
float tds;

#define RXp2 16
#define TXp2 17

//RELAY PIN *************************************************************
int relayPins[] = { 13, 12, 14, 27, 15, 2, 0, 4 };  // Example pins on ESP32
int numRelays = 8;
#define relaypinMain 32

//RESET PIN for Arduino*************************************************************
#define RESET_PIN 26
int countBeforeReset = 0;

//BUZZER PIN for Arduino*************************************************************
#define BUZZER_PIN 25

//----------------------------------- INIT WIFI ------------------------------------------
void initWiFi() {
  WiFi.mode(WIFI_STA);
  // Add list of wifi networks
  wifiMulti.addAP("HUAWEI-2.4G-8NqA", "p69CE656");
  wifiMulti.addAP("Jay", "123456789jay");
  wifiMulti.addAP("Jay-Extender", "888888889");

  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0) {
    Serial.println("no networks found");
  } else {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      wifi_ssid = String(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");

      delay(50);
    }
  }

  // Connect to Wi-Fi using wifiMulti (connects to the SSID with strongest connection)
  Serial.println("Connecting Wifi...");
  if (wifiMulti.run() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    esp32_ip = String(WiFi.localIP());
  }
}

void initReconnectWifit() {
  if (wifiMulti.run(connectTimeoutMs) == WL_CONNECTED) {
    Serial.print("WiFi connected: ");
    Serial.print(WiFi.SSID());
    Serial.print(" ");
    Serial.println(WiFi.RSSI());
  } else {
    Serial.println("WiFi not connected!");
  }
  delay(1000);
}

//----------------------------------- INIT FIREBASE ------------------------------------------
void initFirebase() {
  // Assign the api key (required)
  config.api_key = API_KEY;
  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);
  fbdo.setBSSLBufferSize(8192 /* Rx buffer size in bytes from 512 - 16384 */, 1024 /* Tx buffer size in bytes from 512 - 16384 */);
  fbdo.setResponseSize(4096);  //4096
  // Assign the callback function for the long running token generation task
  config.token_status_callback = tokenStatusCallback;
  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;
  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  signupOK = true;

  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  user_uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.print(user_uid);
}

//----------------------------------- SETUP ------------------------------------------
void setup() {
  Serial.begin(115200);  // Serial communication with the computer

  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, HIGH);
  pinMode(BUZZER_PIN, OUTPUT);
  buzzerfunction(1);

  //Relay PIN---------------
  pinMode(relaypinMain, OUTPUT);
  delay(1000);
  digitalWrite(relaypinMain, HIGH);
  digitalWrite(relaypinMain, LOW);
  delay(1000);

  for (int i = 0; i < numRelays; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], HIGH);  //OFF
  }                                    //Relay PIN-----------------

  Serial2.begin(9600, SERIAL_8N1, RXp2, TXp2);  // Serial communication with the Arduino
  Serial.println("Loading");
  initWiFi();  //initialize wifi
  // Configure time with NTP
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  while ((now = time(nullptr)) < 1577836800) {  // January 1, 2020
    Serial.println("Waiting for time to be set...");
    delay(2000);
  }

  initFirebase();           //initialize firebase
  CheckDateTime();          //check date time
  firestoreSendSettings();  //send default settings

  Serial.println("Loading");
  delay(2000);
}

//----------------------------------- Control Variables ------------------------------------------
std::string fsWaterTank = "watertank";
bool watertankControl, watertankManual;
std::string fsMisting = "misting";
bool mistingControl, mistingManual;
std::string fsIrrigation = "irrigation";
bool irrigationControl, irrigationManual;
std::string fsFan = "fan";
bool fanControl, fanManual;
std::string fsLight = "light";
bool lightControl, lightManual;

int watertanklevel;
int mistingfrom, mistingto, mistingduration;
int irrigationfrom, irrigationto, irrigationduration;
int fanfrom, fanto, fanduration;
int lightfrom, lightto, lightduration;

unsigned long mistingStartTime = 0;
unsigned long irrigationStartTime = 0;
unsigned long fanStartTime = 0;
unsigned long lightStartTime = 0;

bool mistingflag, irrigationflag, fanflag, lightflag;

//------------------------------------ LOOP -----------------------------------------
void loop() {
  initReconnectWifit();
  if (Firebase.isTokenExpired()) {
    Firebase.refreshToken(&config);
    Serial.println("Refresh token");
  }
  //Monitor Functions
  CheckDateTime();  //check date time
  delay(1000);
  uartReceive();  //UART get sensor data
  delay(1000);

  firestoreSendMonitor();  // send monitor sensors data
  delay(1000);
  firebaseSendData();  // send data firebase
  delay(1000);
  CheckReportToSend();  //check and send report

  //Control Functions
  WaterTankControl(watertankControl, watertankManual, fsWaterTank.c_str(), watertanklevel);
  Control(mistingControl, mistingManual, fsMisting.c_str(), mistingfrom, mistingto, mistingduration);
  Control(irrigationControl, irrigationManual, fsIrrigation.c_str(), irrigationfrom, irrigationto, irrigationduration);
  Control(fanControl, fanManual, fsFan.c_str(), fanfrom, fanto, fanduration);
  Control(lightControl, lightManual, fsLight.c_str(), lightfrom, lightto, lightduration);

  relayfunction();
  delay(1000);

  Serial.print("Reset Arduino in: ");
  Serial.println(countBeforeReset);
  if (countBeforeReset == 20) {
    resetArduino();
    countBeforeReset = 0;
    Serial.println("Arduino has been Reset");
  }

  //Speaker Functions
  buzzerfunction(2);
  Serial.println("----------------------------------------------------------------------");
}
//------------------------------------ Reset Function -----------------------------------------
void resetArduino() {
  digitalWrite(RESET_PIN, LOW);
  delay(4000);
  digitalWrite(RESET_PIN, HIGH);
  delay(1000);
}

//------------------------------------ Relay Function -----------------------------------------
void relayfunction() {

  if (watertankControl) {              // relay pin 0
                                       //automatic control
    digitalWrite(relayPins[0], HIGH);  //off
    if (waterlevel == watertanklevel) {
      digitalWrite(relayPins[0], LOW);  //on
    } else {
      digitalWrite(relayPins[0], HIGH);  //off
    }
  } else {
    if (watertankManual) {              //manual countrol
      digitalWrite(relayPins[0], LOW);  //on
    } else {
      digitalWrite(relayPins[0], HIGH);  //off
    }
  }

  if (mistingControl) {  // relay pin 1
    //automatic control
    controlSystem(mistingfrom, mistingto, mistingStartTime, mistingduration, relayPins[1], mistingflag);
  } else {
    if (mistingManual) {                //manual countrol
      digitalWrite(relayPins[1], LOW);  //on
    } else {
      digitalWrite(relayPins[1], HIGH);  //off
    }
  }

  if (irrigationControl) {  // relay pin 2
    //automatic control
    controlSystem(irrigationfrom, irrigationto, irrigationStartTime, irrigationduration, relayPins[2], irrigationflag);
  } else {
    if (irrigationManual) {             //manual countrol
      digitalWrite(relayPins[2], LOW);  //on
    } else {
      digitalWrite(relayPins[2], HIGH);  //off
    }
  }

  if (fanControl) {  // relay pin 3
    //automatic control
    controlSystem(fanfrom, fanto, fanStartTime, fanduration, relayPins[3], fanflag);
  } else {
    if (fanManual) {                    //manual countrol
      digitalWrite(relayPins[3], LOW);  //on
    } else {
      digitalWrite(relayPins[3], HIGH);  //off
    }
  }

  if (lightControl) {  // relay pin 4
    //automatic control
    controlSystem(lightfrom, lightto, lightStartTime, lightduration, relayPins[4], lightflag);
  } else {
    if (lightManual) {                  //manual countrol
      digitalWrite(relayPins[4], LOW);  //on
    } else {
      digitalWrite(relayPins[4], HIGH);  //off
    }
  }

  delay(1000);
}

void controlSystem(int from, int to, unsigned long& startTime, int duration, int relayPin, bool& isOn) {
  unsigned long currentTime = millis();
  unsigned long durationOnMillis = duration * 60000UL;
  unsigned long durationOffMillis = duration * 60000UL;

  if (from <= hour && hour <= to) {
    // Within schedule
    if (!startTime) {
      // If the system is not already running, start it
      digitalWrite(relayPin, LOW);  // Turn on the system
      startTime = currentTime;      // Record start time
      isOn = true;                  // Set flag to indicate the system is on
    }

    // Check if it's time to turn off the system
    if (isOn && (currentTime - startTime >= durationOnMillis)) {
      digitalWrite(relayPin, HIGH);  // Turn off the system
      isOn = false;                  // Update flag to indicate the system is off
      startTime = currentTime;       // Reset start time
    }
    // Check if it's time to turn on the system
    else if (!isOn && (currentTime - startTime >= durationOffMillis)) {
      digitalWrite(relayPin, LOW);  // Turn on the system
      isOn = true;                  // Update flag to indicate the system is on
      startTime = currentTime;      // Reset start time
    }
  } else {
    // Outside schedule
    digitalWrite(relayPin, HIGH);  // Turn off the system
    isOn = false;                  // Update flag to indicate the system is off
    startTime = 0;                 // Reset start time
  }
}


//---------------------------------- Control ----------------------------------------
void Control(bool& controlMode, bool& manualMode, const String& document, int& from, int& to, int& duration) {
  String documentPath = "user/" + user_uid + "/control/" + document + "/";

  if (Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str())) {
    //Serial.printf("Read successful\n%s\n\n", fbdo.payload().c_str());
    FirebaseJsonData jsonData;
    FirebaseJson json;
    json.setJsonData(fbdo.payload().c_str());

    if (json.get(jsonData, "fields/controlmode/booleanValue")) {
      controlMode = jsonData.to<bool>();
    } else {
      Serial.println("Error: 'controlmode' field not found or has an invalid type");
    }

    if (json.get(jsonData, "fields/manualmode/booleanValue")) {
      manualMode = jsonData.to<bool>();
    } else {
      Serial.println("Error: 'manualmode' field not found or has an invalid type");
    }

    //--------------------------------------------
    if (json.get(jsonData, "fields/from/integerValue")) {
      from = jsonData.to<int>();
    } else {
      Serial.println("Error: 'from' field not found or has an invalid type");
    }

    if (json.get(jsonData, "fields/to/integerValue")) {
      to = jsonData.to<int>();
    } else {
      Serial.println("Error: 'to' field not found or has an invalid type");
    }
    if (json.get(jsonData, "fields/duration/integerValue")) {
      duration = jsonData.to<int>();
    } else {
      Serial.println("Error: 'duration' field not found or has an invalid type");
    }
  } else {
    Serial.print("Error in Firebase operation: ");
    Serial.println(fbdo.errorReason());
  }
}

void WaterTankControl(bool& controlMode, bool& manualMode, const String& document, int& level) {
  String documentPath = "user/" + user_uid + "/control/" + document + "/";

  if (Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str())) {
    //Serial.printf("Read successful\n%s\n\n", fbdo.payload().c_str());
    FirebaseJsonData jsonData;
    FirebaseJson json;
    json.setJsonData(fbdo.payload().c_str());

    if (json.get(jsonData, "fields/controlmode/booleanValue")) {
      controlMode = jsonData.to<bool>();
    } else {
      Serial.println("Error: 'controlmode' field not found or has an invalid type");
    }

    if (json.get(jsonData, "fields/manualmode/booleanValue")) {
      manualMode = jsonData.to<bool>();
    } else {
      Serial.println("Error: 'manualmode' field not found or has an invalid type");
    }
    if (json.get(jsonData, "fields/waterlevel/integerValue")) {
      level = jsonData.to<int>();
    } else {
      Serial.println("Error: 'waterlevel' field not found or has an invalid type");
    }
  } else {
    Serial.print("Error in Firebase operation: ");
    Serial.println(fbdo.errorReason());
  }
}
//------------------------------------ UART Function -----------------------------------------
void uartReceive() {
  // Check if data is available from the Arduino over UART
  if (Serial2.available()) {
    // Read the data as a JSON string
    String jsonString = Serial2.readStringUntil('\n');

    // Parse JSON
    StaticJsonDocument<128> doc;
    DeserializationError error = deserializeJson(doc, jsonString);

    if (!error) {
      // Extract sensor readings
      watertemp = doc["watertemp"];
      humidity = doc["humidity"];
      airtemp = doc["airtemp"];
      waterlevel = doc["waterlevel"];
      ph = doc["ph"];
      tds = doc["tds"];

      // Print or use the sensor readings
      Serial.print("WaterTemp: ");
      Serial.println(watertemp);
      //Serial.print("°C");

      Serial.print("Humidity: ");
      Serial.println(humidity);

      Serial.print("AirTemp: ");
      Serial.println(airtemp);
      //Serial.print("°C");

      Serial.print("WaterLevel: ");
      Serial.println(waterlevel);

      Serial.print("ph: ");
      Serial.println(ph);

      Serial.print("Tds: ");
      Serial.println(tds);
      //Serial.print("ppm");


    } else {
      Serial.print("JSON parsing failed: ");
      Serial.println(error.c_str());
    }
  }
  //add count to reset counter
  countBeforeReset += 1;
  // Add your code here to process the received sensor data
}

//---------------------------------- Firebase RTDB Send Data----------------------------------------
void firebaseSendData() {
  if (Firebase.isTokenExpired()) {
    Firebase.refreshToken(&config);
    Serial.println("Refresh token");
  }

  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 5000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    if (Firebase.RTDB.setFloat(&fbdo, "" + user_uid + "/Monitor/airtemp", airtemp)) {
      Serial.print(airtemp);
      Serial.print("- successfully saved to: " + fbdo.dataPath());
      Serial.println("(" + fbdo.dataType() + ")");
    } else {
      Serial.println("Air-Temp FAILED: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setFloat(&fbdo, "" + user_uid + "/Monitor/humidity", humidity)) {
      Serial.print(humidity);
      Serial.print("- successfully saved to: " + fbdo.dataPath());
      Serial.println("(" + fbdo.dataType() + ")");
    } else {
      Serial.println("Humidity FAILED: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setFloat(&fbdo, "" + user_uid + "/Monitor/ph", ph)) {
      Serial.print(ph);
      Serial.print("- successfully saved to: " + fbdo.dataPath());
      Serial.println("(" + fbdo.dataType() + ")");
    } else {
      Serial.println("pH FAILED: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setFloat(&fbdo, "" + user_uid + "/Monitor/tds", tds)) {
      Serial.print(tds);
      Serial.print("- successfully saved to: " + fbdo.dataPath());
      Serial.println("(" + fbdo.dataType() + ")");
    } else {
      Serial.println("pH FAILED: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setFloat(&fbdo, "" + user_uid + "/Monitor/water", waterlevel)) {
      Serial.print(waterlevel);
      Serial.print("- successfully saved to: " + fbdo.dataPath());
      Serial.println("(" + fbdo.dataType() + ")");
    } else {
      Serial.println("Water-level FAILED: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setFloat(&fbdo, "" + user_uid + "/Monitor/watertemp", watertemp)) {
      Serial.print(watertemp);
      Serial.print("- successfully saved to: " + fbdo.dataPath());
      Serial.println("(" + fbdo.dataType() + ")");
    } else {
      Serial.println("Water-temp FAILED: " + fbdo.errorReason());
    }
  }
  Serial.println();
}
//---------------------------------- Check Time/DATE ----------------------------------------
void CheckDateTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.printf("Current time: %04d-%02d-%02d %02d:%02d:%02d\n",
                timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                (timeinfo.tm_hour + 8) % 24, timeinfo.tm_min, timeinfo.tm_sec);

  String DateOnly = String(timeinfo.tm_year + 1900) + "-" + ((timeinfo.tm_mon + 1) < 10 ? "0" : "") + String(timeinfo.tm_mon + 1) + "-" + ((timeinfo.tm_mday) < 10 ? "0" : "") + String(timeinfo.tm_mday);

  const char* dateOnlyCStr = DateOnly.c_str();  // Convert Arduino String to const char*
  //std::string Date(dateOnlyCStr);
  Date = dateOnlyCStr;  // Convert const char* to std::string

  //Date = std::to_string(timeinfo.tm_year + 1900) + "-" + std::to_string(timeinfo.tm_mon + 1) + "-" + std::to_string(timeinfo.tm_mday);

  //Set Parameters
  DayToday = timeinfo.tm_mday;
  Month = timeinfo.tm_mon;
  Year = timeinfo.tm_year + 1900;

  hour = (timeinfo.tm_hour + 8) % 24;
  mins = timeinfo.tm_min;
  secs = timeinfo.tm_sec;
}

//---------------------------------- Check Report to Send Data ----------------------------------------
unsigned long lastSendTimestamp = 0;
bool sendReportFlag = false;
void CheckReportToSend() {
  if (hour == 7 || hour == 8 || hour == 9 || hour == 10 || hour == 11 || hour == 12 || hour == 13 || hour == 14 || hour == 15 || hour == 16 || hour == 17 || hour == 18 || hour == 19) {
    if (mins >= 59) {
      sendReportFlag = true;
    } else {
      if (sendReportFlag) {
        if (millis() - lastSendTimestamp >= 60000) {
          firebasefirestoreSendData();
          Serial.println("Report: Sent to Firestore");
          lastSendTimestamp = millis();
          sendReportFlag = false;
          buzzerfunction(3);
        } else {
          Serial.println("Report: Not within the specified time");
        }
      }
    }
  } else {
    Serial.println("Report: Not within the specified time");
  }

  Serial.println("Report Stat: hour:" + String(hour) + " min:" + String(mins) + " secs:" + String(secs));
}

//---------------------------------- FirebaseFirestore SenData ----------------------------------------
void firebasefirestoreSendData() {
  String documentPath = "user/" + user_uid + "/report//";

  FirebaseJson content;
  content.set("fields/date/stringValue", String(Date.c_str()));
  content.set("fields/day/integerValue", String(DayToday).c_str());
  content.set("fields/month/integerValue", String(Month).c_str());
  content.set("fields/year/integerValue", String(Year).c_str());
  content.set("fields/hour/integerValue", String(hour - 1).c_str());  // minus 1hour after passing the hour
  content.set("fields/ph/doubleValue", String(ph).c_str());
  content.set("fields/tds/doubleValue", String(tds).c_str());
  content.set("fields/waterlevel/integerValue", String(waterlevel).c_str());
  content.set("fields/watertemp/doubleValue", String(watertemp).c_str());
  content.set("fields/humidity/doubleValue", String(humidity).c_str());
  content.set("fields/temperature/doubleValue", String(airtemp).c_str());

  if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(), "date,day,month,year,hour,ph,tds,waterlevel,watertemp,humidity,temperature")) {
    //Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
    Serial.println("Firestore: Report Data Sent");
    return;
  } else {
    Serial.println(fbdo.errorReason());
  }

  if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw())) {
    //Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
    Serial.println("Firestore: Report Data Sent");
    return;
  } else {
    Serial.println(fbdo.errorReason());
  }
}


//---------------------------------- FirebaseFirestore SenData Settings----------------------------------------
void firestoreSendSettings() {
  String documentPath = "user/" + user_uid + "/setting/default/";

  FirebaseJson content;
  content.clear();
  content.set("fields/deviceip/stringValue", esp32_ip.c_str());
  content.set("fields/devicessid/stringValue", wifi_ssid.c_str());
  content.set("fields/devicestatus/booleanValue", sdevice);
  content.set("fields/uid/stringValue", user_uid.c_str());
  content.set("fields/date/stringValue", Date.c_str());


  if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(), "deviceip, devicessid, devicestatus, uid, date")) {
    //Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
    Serial.println("Firestore: Settings Data Sent");
    return;
  } else {
    Serial.println(fbdo.errorReason());
  }

  if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw())) {
    //Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
    Serial.println("Firestore: Settings Data Sent");
    return;
  } else {
    Serial.println(fbdo.errorReason());
  }
}

void firestoreSendMonitor() {
  String documentPath = "user/" + user_uid + "/monitor/result/";

  FirebaseJson content;
  content.clear();
  content.set("fields/airtemp/doubleValue", String(airtemp).c_str());
  content.set("fields/humidity/doubleValue", String(humidity).c_str());
  content.set("fields/ph/doubleValue", String(ph).c_str());
  content.set("fields/tds/doubleValue", String(tds).c_str());
  content.set("fields/water/integerValue", String(waterlevel).c_str());
  content.set("fields/watertemp/doubleValue", String(watertemp).c_str());

  if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(), "airtemp, humidity, ph, tds, water, watertemp")) {
    //Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
    Serial.println("Firestore: Sensors Data Sent");
    return;
  } else {
    Serial.println(fbdo.errorReason());
  }

  if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw())) {
    //Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
    Serial.println("Firestore: Sensors Data Sent");
    return;
  } else {
    Serial.println(fbdo.errorReason());
  }
}
//---------------------------------- Buzzer ----------------------------------------
#define NOTE_C4 261
#define NOTE_D4 294
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_G4 392
#define NOTE_A4 440
#define NOTE_B4 494
#define NOTE_C5 523

void buzzerfunction(int tones) {
  if (tones == 1) {  //tone entry or turn on
    tone(BUZZER_PIN, NOTE_C4);
    delay(500);
    tone(BUZZER_PIN, NOTE_D4);
    delay(500);
    tone(BUZZER_PIN, NOTE_E4);
    delay(500);
    tone(BUZZER_PIN, NOTE_F4);
    delay(500);
    tone(BUZZER_PIN, NOTE_G4);
    delay(500);
    tone(BUZZER_PIN, NOTE_A4);
    delay(500);
    tone(BUZZER_PIN, NOTE_B4);
    delay(500);
    tone(BUZZER_PIN, NOTE_C5);
    delay(500);
    noTone(BUZZER_PIN);     // Stop playing the tone
  } else if (tones == 2) {  //tone when finished looping
    tone(BUZZER_PIN, 1000);
    delay(300);
    noTone(BUZZER_PIN);
  } else if (tones == 3) {  // tone when report are sent
    tone(BUZZER_PIN, NOTE_A4);
    delay(500);
    tone(BUZZER_PIN, NOTE_B4);
    delay(500);
    tone(BUZZER_PIN, NOTE_C5);
    delay(500);
    tone(BUZZER_PIN, NOTE_A4);
    delay(500);
    tone(BUZZER_PIN, NOTE_G4);
    delay(500);
    tone(BUZZER_PIN, NOTE_C5);
    delay(500);
    tone(BUZZER_PIN, NOTE_A4);
    delay(500);
    tone(BUZZER_PIN, NOTE_F4);
    delay(500);
    tone(BUZZER_PIN, NOTE_C5);
    delay(500);
    noTone(BUZZER_PIN);
  } else if (tones == 4) {
    tone(BUZZER_PIN, NOTE_C5);
    delay(300);
    tone(BUZZER_PIN, NOTE_F4);
    delay(500);
    noTone(BUZZER_PIN);
  }
}
