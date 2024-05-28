#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define WIFI_SSID "HUAWEI-2.4G-8NqA"
#define WIFI_PASSWORD "p69CE656"
#define API_KEY "AIzaSyCsGkDTOAjOzxHl3qp97PzeeHs63WEn6nc"
#define DATABASE_URL "https://rtdbtest-e1f4a-default-rtdb.asia-southeast1.firebasedatabase.app/"


#define LED1_PIN 12
#define LED2_PIN 14
#define LDR_PIN 36
#define PWMChannel 0

const int freq = 5000;
const int resolution = 8;

FirebaseData fbdo, fbdo_s1, fbdo_s2;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
bool signupOK = false;
int ldrData = 0;
float voltage = 0.0;
int pwmValue = 0;
bool ledStatus = false;

void setup() {
  pinMode(LED2_PIN, OUTPUT);
  ledcSetup(PWMChannel, freq, resolution);
  ledcAttachPin(LED1_PIN, PWMChannel);

  Serial.begin(115200);//serial monitor baud
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("."); delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  if(Firebase.signUp(&config, &auth, "", "")){
    Serial.println("SignUP OK");
    signupOK = true;
  }else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }
  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  //Stream
  if(!Firebase.RTDB.beginStream(&fbdo_s1, "/LED/analog")) 
    Serial.printf("stream 1 begin error, %s\n\n", fbdo_s1.errorReason().c_str());
  
  if(!Firebase.RTDB.beginStream(&fbdo_s2, "/LED/digital")) 
    Serial.printf("stream 2 begin error, %s\n\n", fbdo_s2.errorReason().c_str());
  
}

void loop() {
  if(Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 5000 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();
    // -----------------STORE sensor data to RTBD
    ldrData = analogRead(LDR_PIN);
    voltage = (float)analogReadMilliVolts(LDR_PIN)/1000;
    if(Firebase.RTDB.setInt(&fbdo, "Sensor/ldr_data", ldrData)){
      Serial.println(); Serial.print(ldrData);
      Serial.print("- successfully saved to: " + fbdo.dataPath());
      Serial.println("("+ fbdo.dataType() +")");
    }else{
      Serial.println("FAILED: " + fbdo.errorReason());
    }

    if(Firebase.RTDB.setFloat(&fbdo, "Sensor/voltage", voltage)){
      Serial.print(ldrData);
      Serial.print("- successfully saved to: " + fbdo.dataPath());
      Serial.println("("+ fbdo.dataType() +")");
    }else{
      Serial.println("FAILED: " + fbdo.errorReason());
    }
  }

  //--------------------------------READ data from RTDB onDataChange
  if(Firebase.ready() && signupOK){
    if(!Firebase.RTDB.readStream(&fbdo_s1))
      Serial.printf("stream 1 read error, %s\n\n", fbdo_s1.errorReason().c_str());
    
    if(fbdo_s1.streamAvailable()){
      if(fbdo_s1.dataType() == "int"){
        pwmValue = fbdo_s1.intData();
        Serial.println("Successful Read from " + fbdo_s1.dataPath() + ": " + pwmValue + "(" + fbdo_s1.dataType() + ")");
        ledcWrite(PWMChannel, pwmValue);
      }
    }

    if(!Firebase.RTDB.readStream(&fbdo_s2))
      Serial.printf("stream 2 read error, %s\n\n", fbdo_s2.errorReason().c_str());
    
    if(fbdo_s2.streamAvailable()){
      if(fbdo_s2.dataType() == "boolean"){
        ledStatus = fbdo_s2.boolData();
        Serial.println("Successful Read from " + fbdo_s2.dataPath() + ": " + ledStatus + "(" + fbdo_s2.dataType() + ")");
        digitalWrite(LED2_PIN, ledStatus);
      }
    }
  }
}
