/*
 Hydroponic - Monitoring System
 Author: Jimmy Jucar Jr
*/
#include <OneWire.h>//waterproof sensor library
#include <DallasTemperature.h>//waterproof sensor library
#include <DHT.h>//humidity&temp library
#include <Wire.h>//ultrasonic distance sensor library
#include <LiquidCrystal_I2C.h> // LCD library
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
int phval = 0; 
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
void setup() {
  lcd.init();//LCD
  lcd.backlight();//LCD

  dht.begin();// humidity & temp sensor
  wptsensor.begin();// waterproof Temp sensor
  
  pinMode(trig, OUTPUT);//ultrasonic distance sensor
  pinMode(echo, INPUT);//ultrasonic distance sensor

  Serial.begin(115200);
  Serial.println("Loading");

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
