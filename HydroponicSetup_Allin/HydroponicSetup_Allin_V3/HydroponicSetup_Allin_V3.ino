/*
 Hydroponic - Monitoring System
 Author: Jimmy Jucar Jr
*/
#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <OneWire.h>//waterproof sensor library
#include <DallasTemperature.h>//waterproof sensor library
#include <DHT.h>//humidity&temp library
#include <Wire.h>//ultrasonic distance sensor library
#include <LiquidCrystal_I2C.h> // LCD library
#include <Firebase_ESP_Client.h>//Firebase
#include "addons/TokenHelper.h"//Firebase
#include "addons/RTDBHelper.h"//Firebase
// WIFI **********************************************************
#define WIFI_SSID "HUAWEI-2.4G-8NqA"
#define WIFI_PASSWORD "p69CE656"
#define API_KEY "AIzaSyDrCAGQ8tClxtwseyOsFAbkMwWz_2MTUdI"
#define DATABASE_URL "https://ohcrop-65556-default-rtdb.asia-southeast1.firebasedatabase.app/"

#define USER_EMAIL "jaytschools@gmail.com"
#define USER_PASSWORD "Anoto123"

FirebaseData fbdo, fbdo_s1, fbdo_s2;
FirebaseAuth auth;
FirebaseConfig config;

String uid;

unsigned long sendDataPrevMillis = 0;
bool signupOK = false;
int ldrData = 0;
float voltage = 0.0;
int pwmValue = 0;
bool ledStatus = false;

// LCD Display **********************************************************
int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

// Humidity & Temp SENSOR ************************************************
float airtemp, humidity;
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
int ultrasonic_distance;
int MaxLevel = 9;// change value base on distance
int Level1 = (MaxLevel * 80) / 100;
int Level2 = (MaxLevel * 65) / 100;
int Level3 = (MaxLevel * 55) / 100;
int Level4 = (MaxLevel * 35) / 100;

//----------------------------------- SETUP ------------------------------------------
void initWiFi(){
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);// begin wifi --------------
  Serial.print("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("."); delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  config.database_url = DATABASE_URL;
  if(Firebase.signUp(&config, &auth, "", "")){
    Serial.println("SignUP OK");
    signupOK = true;
  }else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  config.token_status_callback = tokenStatusCallback;
  config.max_token_generation_retry = 5;
  Firebase.begin(&config, &auth);
  

  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.print(uid);

  //if(!Firebase.RTDB.beginStream(&fbdo_s1, "/LED/analog")) 
    //Serial.printf("stream 1 begin error, %s\n\n", fbdo_s1.errorReason().c_str());
  
  //if(!Firebase.RTDB.beginStream(&fbdo_s2, "/LED/digital")) 
    //Serial.printf("stream 2 begin error, %s\n\n", fbdo_s2.errorReason().c_str());
  // ----------------------------------------------------------------

}
void setup() {
  Serial.begin(115200);
  Serial.println("Loading");
  lcd.init();//LCD
  lcd.backlight();//LCD

  initWiFi();
  
  dht.begin();// humidity & temp sensor
  wptsensor.begin();// waterproof Temp sensor
  
  pinMode(trig, OUTPUT);//ultrasonic distance sensor
  pinMode(echo, INPUT);//ultrasonic distance sensor

  lcd.setCursor(3,0);
  lcd.print( "Loading..." );//print to LCD
  delay(2000);
  lcd.clear();//clear LCD
  lcd.setCursor(2,0);
  lcd.print("OHCROP SYSTEM");
  delay(2000);
}
//------------------------------------ LOOP ------------------------------------------
void loop() {
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("OHCROP SYSTEM");

  humidityTemp();
  waterproofTemp();
  pHSensor();
  ultrasonicSensor();
  delay(2000);
  Serial.println("------------------------------------------");
  Serial.println();

  displayLCD();

  if (Firebase.isTokenExpired()){
    Firebase.refreshToken(&config);
    Serial.println("Refresh token");
  }

  if(Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 5000 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();
    if(Firebase.RTDB.setFloat(&fbdo, "Monitor/airtemp/data", airtemp)){
      Serial.println(); Serial.print(airtemp);
      Serial.print("- successfully saved to: " + fbdo.dataPath());
      Serial.println("("+ fbdo.dataType() +")");
    }else{
      Serial.println("Air-Temp FAILED: " + fbdo.errorReason());
    }

    if(Firebase.RTDB.setFloat(&fbdo, "Monitor/humidity/data", humidity)){
      Serial.println(); Serial.print(humidity);
      Serial.print("- successfully saved to: " + fbdo.dataPath());
      Serial.println("("+ fbdo.dataType() +")");
    }else{
      Serial.println("Humidity FAILED: " + fbdo.errorReason());
    }

    if(Firebase.RTDB.setFloat(&fbdo, "Monitor/ph/data", ph_Value)){
      Serial.println(); Serial.print(ph_Value);
      Serial.print("- successfully saved to: " + fbdo.dataPath());
      Serial.println("("+ fbdo.dataType() +")");
    }else{
      Serial.println("pH FAILED: " + fbdo.errorReason());
    }

    if(Firebase.RTDB.setFloat(&fbdo, "Monitor/water/level/data", ultrasonic_distance)){
      Serial.println(); Serial.print(ultrasonic_distance);
      Serial.print("- successfully saved to: " + fbdo.dataPath());
      Serial.println("("+ fbdo.dataType() +")");
    }else{
      Serial.println("Water-level FAILED: " + fbdo.errorReason());
    }

    if(Firebase.RTDB.setFloat(&fbdo, "Monitor/water/temp/data", tempC)){
      Serial.println(); Serial.print(tempC);
      Serial.print("- successfully saved to: " + fbdo.dataPath());
      Serial.println("("+ fbdo.dataType() +")");
    }else{
      Serial.println("Water-temp FAILED: " + fbdo.errorReason());
    }
  }

  Serial.println();
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
  lcd.print(airtemp); delay(1000);

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
  lcd.print(ultrasonic_distance); delay(1000);
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
