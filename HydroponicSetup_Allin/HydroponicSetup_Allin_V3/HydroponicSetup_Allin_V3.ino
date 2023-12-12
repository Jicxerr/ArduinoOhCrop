/*
 Hydroponic - Monitoring System
 Author: Jimmy Jucar Jr
*/
#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
  #include <WiFiMulti.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <time.h>//Time
#include <OneWire.h>//waterproof sensor library
#include <DallasTemperature.h>//waterproof sensor library
#include <DHT.h>//humidity&temp library
#include <Wire.h>//ultrasonic distance sensor library
#include <EEPROM.h>//TDS eeprom calibration
#include "GravityTDS.h"//TDS library
#include <LiquidCrystal_I2C.h> // LCD library
#include <Firebase_ESP_Client.h>//Firebase
#include "addons/TokenHelper.h"//Firebase
#include "addons/RTDBHelper.h"//Firebase

//#define WIFI_SSID "HUAWEI-2.4G-8NqA"
//#define WIFI_PASSWORD "p69CE656"
#define API_KEY "AIzaSyDrCAGQ8tClxtwseyOsFAbkMwWz_2MTUdI"
#define DATABASE_URL "https://ohcrop-65556-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define FIREBASE_PROJECT_ID "ohcrop-65556"

#define USER_EMAIL "jay@gmail.com"
#define USER_PASSWORD "123456"

FirebaseData fbdo, fbdo_s1, fbdo_s2;
FirebaseAuth auth;
FirebaseConfig config;

// Setting **********************************************************
bool sph = true;
bool stds = true;
bool swater = true;
bool swatertemp = true;
bool shumidity = true;
bool stemp = true;
bool sdevice = true;
String user_uid; //UserID
String wifi_ssid; // wifi_ssid
String esp32_ip; // esp32_ip address

//**********************************************************
unsigned long sendDataPrevMillis = 0;
bool signupOK = false;
int ldrData = 0;
float voltage = 0.0;
int pwmValue = 0;
bool ledStatus = false;
int count = 0;
// TDS **********************************************************
#include <EEPROM.h>
#include "GravityTDS.h"
#define TdsSensorPin A0
GravityTDS gravityTds;
float tdstemperature = 25,tdsValue = 0;

// LCD Display **********************************************************
int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

// Humidity & Temp SENSOR ************************************************
float airtemp = 0;// airtemp value,
float humidity = 0;// humidity value
DHT dht(15, DHT11);//GPIO5

//Waterproof Temp SENSOR *************************************************
#define WPTempSensor 4 //GPIO4
OneWire oneWire(WPTempSensor);
DallasTemperature wptsensor(&oneWire);
float tempC; // temperature in Celsius
float tempF; // temperature in Fahrenheit

// PH SENSOR *************************************************************
#define PHSensor 35
float calibration_value = 21.34 + 0.77;//21.34 2.28
unsigned long int avgval; 
int buffer_arr[10],temp;
float ph_Value; //Ph value result

// Ultrasonic Distance SENSOR ********************************************
#define trig 12
#define echo 14
int ultrasonic_distance; //ultrasonic distance value
int MaxLevel = 9;// change value base on distance
int Level1 = (MaxLevel * 80) / 100;
int Level2 = (MaxLevel * 65) / 100;
int Level3 = (MaxLevel * 55) / 100;
int Level4 = (MaxLevel * 35) / 100;

// WIFI **********************************************************
WiFiMulti wifiMulti;
// WiFi connect timeout per AP. Increase when connecting takes longer.
const uint32_t connectTimeoutMs = 10000;

// DATA/TIME //reportsData**********************************************************
struct tm timeinfo;
int hour, DayToday;
int mins, secs;
std::string Date;



//----------------------------------- INIT WIFI ------------------------------------------
void initWiFi(){
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
  } 
  else {
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
      Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
      
      delay(50);
    }
  }

  // Connect to Wi-Fi using wifiMulti (connects to the SSID with strongest connection)
  Serial.println("Connecting Wifi...");
  if(wifiMulti.run() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    esp32_ip = String(WiFi.localIP());
  }
}
  // For single connection only
  // WiFi.begin(WIFI_SSID, WIFI_PASSWORD);// set ssid pass wifi
  // Serial.print("Connecting to Wi-Fi");

  // while (WiFi.status() != WL_CONNECTED) {
  //   Serial.print("."); delay(300);
  // }
  // Serial.println();
  // Serial.print("Connected with IP: ");
  
  // Serial.println(WiFi.localIP());
  // Serial.println();


void initReconnectWifit(){
  if (wifiMulti.run(connectTimeoutMs) == WL_CONNECTED) {
    Serial.print("WiFi connected: ");
    Serial.print(WiFi.SSID());
    Serial.print(" ");
    Serial.println(WiFi.RSSI());
  }
  else {
    Serial.println("WiFi not connected!");
  }
  delay(1000);
}
//----------------------------------- INIT FIREBASE ------------------------------------------
void initFirebase(){
  // Assign the api key (required)
  config.api_key = API_KEY;
  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;

  // if(Firebase.signUp(&config, &auth, "", "")){
  //   Serial.println("SignUP OK");
  //   signupOK = true;
  // }else{
  //   Serial.printf("%s\n", config.signer.signupError.message.c_str());
  // }

  Firebase.reconnectWiFi(true);
  fbdo.setBSSLBufferSize(8192 /* Rx buffer size in bytes from 512 - 16384 */, 1024 /* Tx buffer size in bytes from 512 - 16384 */);
  fbdo.setResponseSize(4096);//4096
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
  Serial.begin(115200);//serial begin
  Serial.println("Loading");//print lcd loading
  lcd.init();//LCD
  lcd.backlight();//LCD

  initWiFi();//initialize wifi

  // Configure time with NTP
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  time_t now;
  while ((now = time(nullptr)) < 1577836800) {  // January 1, 2020
    Serial.println("Waiting for time to be set...");
    delay(2000);
  }

  initFirebase();//initialize firebase
  
  dht.begin();// humidity & temp sensor
  wptsensor.begin();// waterproof Temp sensor
  
  pinMode(trig, OUTPUT);//ultrasonic distance sensor
  pinMode(echo, INPUT);//ultrasonic distance sensor

  gravityTds.setPin(TdsSensorPin);//TDS Sensor
  gravityTds.setAref(5.0);  //reference voltage on ADC, default 5.0V on Arduino UNO
  gravityTds.setAdcRange(1024);  //1024 for 10bit ADC;4096 for 12bit ADC
  gravityTds.begin();  //initialization

  lcd.setCursor(3,0);
  lcd.print( "Loading..." );//print to LCD
  delay(2000);
  lcd.clear();//clear LCD
  lcd.setCursor(2,0);
  lcd.print("OHCROP SYSTEM");
  delay(2000);
  CheckDateTime();
  firestoreSendSettings();
  delay(2000);
}
//------------------------------------ LOOP ------------------------------------------
void loop() {
  initReconnectWifit();
  if (Firebase.isTokenExpired()){
    Firebase.refreshToken(&config);
    Serial.println("Refresh token");
  }

  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("OHCROP SYSTEM");
  delay(1000);
  humidityTemp();
  delay(1000);
  waterproofTemp();
  delay(1000);
  pHSensor();
  delay(1000);
  tDsSensor();
  delay(1000);
  ultrasonicSensor();
  delay(1000);
  Serial.println("------------------------------------------");
  Serial.println();

  displayLCD();

  firebaseSendData();
  CheckDateTime();
  delay(1000);
  CheckReportToSend();
  delay(1000);
  Serial.println();
}
//---------------------------------- Firebase RTDB Send Data----------------------------------------
void firebaseSendData(){
  if (Firebase.isTokenExpired()){
    Firebase.refreshToken(&config);
    Serial.println("Refresh token");
  }

  if(Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 5000 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();
    
    if(Firebase.RTDB.setFloat(&fbdo, ""+user_uid+"/Monitor/airtemp", airtemp)){
      Serial.println(); Serial.print(airtemp);
      Serial.print("- successfully saved to: " + fbdo.dataPath());
      Serial.println("("+ fbdo.dataType() +")");
    }else{
      Serial.println("Air-Temp FAILED: " + fbdo.errorReason());
    }

    if(Firebase.RTDB.setFloat(&fbdo, ""+user_uid+"/Monitor/humidity", humidity)){
      Serial.println(); Serial.print(humidity);
      Serial.print("- successfully saved to: " + fbdo.dataPath());
      Serial.println("("+ fbdo.dataType() +")");
    }else{
      Serial.println("Humidity FAILED: " + fbdo.errorReason());
    }

    if(Firebase.RTDB.setFloat(&fbdo, ""+user_uid+"/Monitor/ph", ph_Value)){
      Serial.println(); Serial.print(ph_Value);
      Serial.print("- successfully saved to: " + fbdo.dataPath());
      Serial.println("("+ fbdo.dataType() +")");
    }else{
      Serial.println("pH FAILED: " + fbdo.errorReason());
    }

    if(Firebase.RTDB.setFloat(&fbdo, ""+user_uid+"/Monitor/tds", tdsValue)){
      Serial.println(); Serial.print(tdsValue);
      Serial.print("- successfully saved to: " + fbdo.dataPath());
      Serial.println("("+ fbdo.dataType() +")");
    }else{
      Serial.println("pH FAILED: " + fbdo.errorReason());
    }

    if(Firebase.RTDB.setFloat(&fbdo, ""+user_uid+"/Monitor/water", ultrasonic_distance)){
      Serial.println(); Serial.print(ultrasonic_distance);
      Serial.print("- successfully saved to: " + fbdo.dataPath());
      Serial.println("("+ fbdo.dataType() +")");
    }else{
      Serial.println("Water-level FAILED: " + fbdo.errorReason());
    }

    if(Firebase.RTDB.setFloat(&fbdo, ""+user_uid+"/Monitor/watertemp", tempC)){
      Serial.println(); Serial.print(tempC);
      Serial.print("- successfully saved to: " + fbdo.dataPath());
      Serial.println("("+ fbdo.dataType() +")");
    }else{
      Serial.println("Water-temp FAILED: " + fbdo.errorReason());
    }
  }
  delay(1000);
}
//---------------------------------- Check Time/DATE ----------------------------------------
void CheckDateTime(){
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.printf("Current time: %04d-%02d-%02d %02d:%02d:%02d\n",
                timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                timeinfo.tm_hour-4, timeinfo.tm_min, timeinfo.tm_sec);
  
  Date = std::to_string(timeinfo.tm_year + 1900) + "-" + std::to_string(timeinfo.tm_mon + 1) + "-" + std::to_string(timeinfo.tm_mday);
  DayToday = timeinfo.tm_mday;
  
  if(timeinfo.tm_hour == 12){
    hour = 12;
  }else if(timeinfo.tm_hour == 1){
    hour = 1;
  }
  else if(timeinfo.tm_hour == 2){
    hour = 2;
  }
  else if(timeinfo.tm_hour == 3){
    hour = 3;
  }
  else if(timeinfo.tm_hour == 4){
    hour = 4;
  }else{
    hour = timeinfo.tm_hour-4;
  }
  
  mins = timeinfo.tm_min;
  secs = timeinfo.tm_sec;
}
//---------------------------------- Check Report to Send Data ----------------------------------------
void CheckReportToSend(){
  if(mins == 59 && secs > 45){//send report within this time range
    firebasefirestoreSendData();
    if (hour == 7 || hour == 8 || hour == 9 || hour == 10 || hour == 11 || hour == 12 || hour == 13 || hour == 14 || hour == 15 || hour == 16 || hour == 17) {
        firebasefirestoreSendData();
        Serial.println("Report: Sent to Firestore");
    } else {
        Serial.println("Report: Not Exact Time");
    }
  }
}
//---------------------------------- FirebaseFirestore SenData ----------------------------------------
void firebasefirestoreSendData(){
  String documentPath = "user/"+user_uid+"/report//";

  FirebaseJson content;

  content.set("fields/date/stringValue", String(Date.c_str()));
  content.set("fields/day/integerValue", String(DayToday).c_str());
  content.set("fields/hour/integerValue", String(hour).c_str());
  content.set("fields/ph/doubleValue", String(ph_Value).c_str());
  content.set("fields/tds/doubleValue", String(tdsValue).c_str());
  content.set("fields/waterlevel/doubleValue", String(ultrasonic_distance).c_str());
  content.set("fields/watertemp/doubleValue", String(tempC).c_str());
  content.set("fields/humidity/doubleValue", String(humidity).c_str());
  content.set("fields/temperature/doubleValue", String(airtemp).c_str());

  if(Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(), "date,day,hour,ph,tds,waterlevel,watertemp,humidity,temperature")){
    Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
    return;
  }else{
    Serial.println(fbdo.errorReason());
  }

  if(Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw())){
    Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
    return;
  }else{
    Serial.println(fbdo.errorReason());
  }

}

void firestoreSendSettings(){
  String documentPath = "user/"+user_uid+"/setting/default/";

  FirebaseJson content;
  content.clear();
  content.set("fields/deviceip/stringValue", esp32_ip.c_str());
  content.set("fields/devicessid/stringValue", wifi_ssid.c_str());
  content.set("fields/devicestatus/booleanValue", sdevice);
  content.set("fields/ph/booleanValue", sph);
  content.set("fields/tds/booleanValue", stds);
  content.set("fields/water/booleanValue", swater);
  content.set("fields/watertemp/booleanValue", swatertemp);
  content.set("fields/humidity/booleanValue", shumidity);
  content.set("fields/airtemp/booleanValue", stemp);

  if(Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(), "deviceip, devicessid, devicestatus, ph, tds, water, watertemp, humidity, airtemp")){
    Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
    return;
  }else{
    Serial.println(fbdo.errorReason());
  }

  if(Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw())){
    Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
    return;
  }else{
    Serial.println(fbdo.errorReason());
  }

}

//---------------------------------- Functions ----------------------------------------
void displayLCD(){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("HUMIDITY:");
  lcd.setCursor(10, 0);
  lcd.print(humidity);
  lcd.setCursor(0, 1);
  lcd.print("AIR TEMP:");
  lcd.setCursor(10, 1);
  lcd.print(airtemp);
  delay(1000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WATER");
  lcd.setCursor(9, 0);
  lcd.print(tempC);
  lcd.setCursor(15, 0);
  lcd.print("C"); 
  lcd.setCursor(0, 1);
  lcd.print("TEMP:");
  lcd.setCursor(9, 1);
  lcd.print(tempF); 
  lcd.setCursor(15, 1);
  lcd.print("F"); 
  delay(1000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("PH VALUE:");
  lcd.setCursor(10, 0);
  lcd.print(ph_Value);
  lcd.setCursor(0, 1);
  lcd.print("WATER LEVEL:");
  lcd.setCursor(12, 1);
  lcd.print(ultrasonic_distance);
  delay(1000);
}

void humidityTemp(){
  airtemp = dht.readTemperature();
  humidity = dht.readHumidity();
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print("%");
  Serial.println();
  Serial.print("Air Temp: ");
  Serial.print(airtemp);
  Serial.println();
}

void waterproofTemp(){
  wptsensor.requestTemperatures();
  tempC = wptsensor.getTempCByIndex(0);  // read temperature in °C
  tempF = tempC * 9 / 5 + 32; // convert °C to °F
  Serial.print("Water Temperature: ");
  Serial.print(tempC);    // print the temperature in °C
  Serial.print("°C");
  Serial.print("  ~  ");  // separator between °C and °F
  Serial.print(tempF);    // print the temperature in °F
  Serial.println("°F");
  //Serial.println();
}

void pHSensor(){
  for(int i=0;i<10;i++) { 
  buffer_arr[i]=analogRead(PHSensor);
  delay(30);
  }
  for(int i=0;i<9;i++){
    for(int j=i+1;j<10;j++){
      if(buffer_arr[i]>buffer_arr[j]){
        temp=buffer_arr[i];
        buffer_arr[i]=buffer_arr[j];
        buffer_arr[j]=temp;
      }
    }
  }
  avgval=0;
  for(int i=2;i<8;i++)
  avgval+=buffer_arr[i];
  float volt=(float)avgval*3.3/4096.0/6;
  ph_Value = -5.70 * volt + calibration_value;

  Serial.print("pH Value:  "); Serial.print(ph_Value);
  Serial.println();
}

void tDsSensor(){
  //temperature = readTemperature();  //add your temperature sensor and read it
  //tdstemperature = tempC;
  gravityTds.setTemperature(tdstemperature);  // set the temperature and execute temperature compensation
  gravityTds.update();  //sample and calculate
  tdsValue = gravityTds.getTdsValue();  // then get the value
  Serial.print("TDS:  "); Serial.print(tdsValue,0);
  Serial.print("ppm");
  //Serial.println();
}

void ultrasonicSensor(){
  digitalWrite(trig, LOW);
  delayMicroseconds(4);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long t = pulseIn(echo, HIGH);
  ultrasonic_distance = t / 29 / 2;

  Serial.println();
  if (Level1 <= ultrasonic_distance) {
    Serial.print("Water Level 1: "); Serial.print(ultrasonic_distance);
  } else if (Level2 <= ultrasonic_distance && Level1 > ultrasonic_distance) {
    Serial.print("Water Level 2: "); Serial.print(ultrasonic_distance);
  } else if (Level3 <= ultrasonic_distance && Level2 > ultrasonic_distance) {
    Serial.print("Water Level 3: "); Serial.print(ultrasonic_distance);
  } else if (Level4 <= ultrasonic_distance && Level3 > ultrasonic_distance) {
    Serial.print("Water Level 4: "); Serial.print(ultrasonic_distance);
  }
  Serial.println();
}
