//ARDUINO UNO R3 CODE FOR OHCROP SYSTEM (ARDUINO WITH ESP32 UART Communication)
//Author: Jimmy Jucar Jr.
#include <Arduino.h>
#include <ArduinoJson.h>//uart
#include <OneWire.h>//waterproof sensor library
#include <DallasTemperature.h>//waterproof sensor library
#include <DHT.h>//humidity&temp library
#include <Wire.h>//ultrasonic distance sensor library
#include <LiquidCrystal_I2C.h> // LCD library
#include <EEPROM.h>//TDS library
#include "GravityTDS.h"//TDS library
#include <SoftwareSerial.h>
#include <avr/wdt.h>//watchdog

SoftwareSerial SerialESP32(0, 1);// RX, TX

// LCD Display **********************************************************
int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

// Humidity & Temp SENSOR ************************************************
float airtemp = 0;// airtemp value,
float humidity = 0;// humidity value
DHT dht(9, DHT11);//

//Waterproof Temp SENSOR *************************************************
#define WPTempSensor 8 //
OneWire oneWire(WPTempSensor);
DallasTemperature wptsensor(&oneWire);
float tempC; // temperature in Celsius
float tempF; // temperature in Fahrenheit

// Ultrasonic Distance SENSOR ********************************************
#define trig 11
#define echo 10
int ultrasonic_distance; //ultrasonic distance value
int MaxLevel = 9;// change value base on distance
int Level1 = (MaxLevel * 80) / 100;
int Level2 = (MaxLevel * 65) / 100;
int Level3 = (MaxLevel * 55) / 100;
int Level4 = (MaxLevel * 35) / 100;


//RELAY PIN *************************************************************
int RelayPin1 = 6;
int RelayPin2 = 7;

// PH SENSOR *************************************************************
#define PHSensor 12
float calibration_value = 21.34 + 0.77;//21.34 2.28
unsigned long int avgval; 
int buffer_arr[10],temp;
float ph_Value; //Ph value result

// TDS SENSOR ************************************************************
#define TdsSensorPin A0
GravityTDS gravityTds;
float tdsTemperature = 25,tdsValue = 0;

//----------------------------------- SETUP ------------------------------------------
void setup() {
  Serial.println("Loading...");
  Serial.begin(9600);//serial begin
  SerialESP32.begin(9600); // SoftwareSerial communication with the ESP32
  
  lcd.init();//LCD
  lcd.backlight();//LCD

  //SENSORS SETUP
	pinMode(RelayPin1, OUTPUT);//Relay
  pinMode(RelayPin2, OUTPUT);//Relay
  dht.begin();// humidity & temp sensor
  wptsensor.begin();// waterproof Temp sensor
  pinMode(trig, OUTPUT);//ultrasonic distance sensor
  pinMode(echo, INPUT);//ultrasonic distance sensor
  //TDS
  gravityTds.setPin(TdsSensorPin);
  gravityTds.setAref(5.0);  //reference voltage on ADC, default 5.0V on Arduino UNO
  gravityTds.setAdcRange(1024);  //1024 for 10bit ADC;4096 for 12bit ADC
  gravityTds.begin();  //initialization

  //RELAY OFF MAIN SENSOR TDS & PH
  digitalWrite(RelayPin1, HIGH);
  digitalWrite(RelayPin2, HIGH);

  //print to LCD
  lcd.setCursor(3,0);
  lcd.print( "Loading..." );
  delay(2000);
  lcd.clear();//clear LCD
  lcd.setCursor(2,0);
  lcd.print("OHCROP SYSTEM");
  
  Serial.println("Starting...");
  delay(2000);
}

//------------------------------------ LOOP ------------------------------------------
void loop() {
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("OHCROP SYSTEM");
  //delay(1000);
  humidityTemp();
  delay(1000);
  waterproofTemp();
  delay(1000);
  ultrasonicSensor();
Profile picture
Profile picture


  delay(1000);

	//RELAY OFF MAIN SENSOR TDS & PH
  digitalWrite(RelayPin1, HIGH);
  digitalWrite(RelayPin2, HIGH);

	digitalWrite(RelayPin1, LOW);
  delay(1000);
  tdsSensor(); //TDS Sensor
	delay(3000);
	digitalWrite(RelayPin1, HIGH);
  delay(1000);
  digitalWrite(RelayPin2, LOW);
  delay(1000);
  pHSensor();// PH Sensor
	delay(3000);
	digitalWrite(RelayPin2, HIGH);

  displayLCD();// LCD Display
  //Serial.print("Sending...");
  uart(); // call UART
  delay(1000);
  //Serial.println("done...");
  //Serial.println();
}

//------------------------------------ UART Function -----------------------------------------
void uart(){
  // Create a JSON object and add sensor readings
  StaticJsonDocument<128> doc;
  doc["watertemp"] = tempC;
  doc["humidity"] = humidity;
  doc["airtemp"] = airtemp;
  doc["waterlevel"] = ultrasonic_distance;
  doc["ph"] = ph_Value;
  doc["tds"] = tdsValue;

  // Serialize JSON to a string
  String jsonString;
  serializeJson(doc, jsonString);

  // Send JSON string to the ESP32 over UART
  SerialESP32.println(jsonString);
  Serial.println(jsonString);
}


//---------------------------------- Sensors Function ----------------------------------------
void displayLCD(){// LCD
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

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("TDS VALUE:");
  lcd.setCursor(10, 0);
  lcd.print(tdsValue);
  lcd.setCursor(0, 1);
  delay(1000);
}

void humidityTemp(){//Humidity & AirTemp
  airtemp = dht.readTemperature();
  humidity = dht.readHumidity();
  // Serial.print("Humidity: ");
  // Serial.print(humidity);
  // Serial.print("%");
  // Serial.println();
  // Serial.print("Air Temp: ");
  // Serial.print(airtemp);
  // Serial.println();
}

void waterproofTemp(){//Water Temp Sensor
  wptsensor.requestTemperatures();
  tempC = wptsensor.getTempCByIndex(0);  // read temperature in °C
  tempF = tempC * 9 / 5 + 32; // convert °C to °F
  // Serial.print("Water Temperature: ");
  // Serial.print(tempC);    // print the temperature in °C
  // Serial.print("°C");
  // Serial.print("  ~  ");  // separator between °C and °F
  // Serial.print(tempF);    // print the temperature in °F
  // Serial.println("°F");
  // Serial.println();
}

void ultrasonicSensor(){// Ultrasonic Distance Sensor
  digitalWrite(trig, LOW);
  delayMicroseconds(4);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long t = pulseIn(echo, HIGH);
  ultrasonic_distance = t / 29 / 2;

  // Serial.println();
  // if (Level1 <= ultrasonic_distance) {
  //   Serial.print("Water Level 1: "); Serial.print(ultrasonic_distance);
  // } else if (Level2 <= ultrasonic_distance && Level1 > ultrasonic_distance) {
  //   Serial.print("Water Level 2: "); Serial.print(ultrasonic_distance);
  // } else if (Level3 <= ultrasonic_distance && Level2 > ultrasonic_distance) {
  //   Serial.print("Water Level 3: "); Serial.print(ultrasonic_distance);
  // } else if (Level4 <= ultrasonic_distance && Level3 > ultrasonic_distance) {
  //   Serial.print("Water Level 4: "); Serial.print(ultrasonic_distance);
  // }
  // Serial.println();
}

void pHSensor(){// PH Sensor
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

  // Serial.print("pH Value:  "); Serial.print(ph_Value);
  // Serial.println();
}

void tdsSensor(){//TDS Sensor
  gravityTds.setTemperature(tdsTemperature);  // set the temperature and execute temperature compensation
  gravityTds.update();  //sample and calculate
  tdsValue = gravityTds.getTdsValue();  // then get the value
  // Serial.print(tdsValue,0);
  // Serial.println("ppm");
}


