/*
  Rui Santos
  Complete project details at our blog: https://RandomNerdTutorials.com/esp32-esp8266-firebase-authentication/
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
  Based in the Authenticatiions Examples by Firebase-ESP-Client library by mobizt: https://github.com/mobizt/Firebase-ESP-Client/tree/main/examples/Authentications
*/

#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "HUAWEI-2.4G-8NqA"
#define WIFI_PASSWORD "p69CE656"

// Insert Firebase project API Key
#define API_KEY "AIzaSyDrCAGQ8tClxtwseyOsFAbkMwWz_2MTUdI"
#define DATABASE_URL "https://ohcrop-65556-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define FIREBASE_PROJECT_ID "ohcrop-65556"

// Insert Authorized Email and Corresponding Password
#define USER_EMAIL "jay@gmail.com"
#define USER_PASSWORD "123456"

// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variable to save USER UID
String uid;




int airtemp = 1;
bool signupOK = false;
unsigned long sendDataPrevMillis = 0;
int count = 0;

// Initialize WiFi
void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  Serial.println();
}

void batchCallback(const char *data)
{
    Serial.print(data);
}

void setup(){
  Serial.begin(115200);
  
  // Initialize WiFi
  initWiFi();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  // Assign the api key (required)
  config.api_key = API_KEY;

  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);
  fbdo.setBSSLBufferSize(8192 /* Rx buffer size in bytes from 512 - 16384 */, 1024 /* Tx buffer size in bytes from 512 - 16384 */);
  fbdo.setResponseSize(4096);//4096

  // Assign the callback function for the long running token generation task
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.print(uid);
 
  // if(Firebase.signUp(&config, &auth, USER_EMAIL, USER_PASSWORD)){
  //   Serial.println("SignUP OK");
    
  // }else{
  //   Serial.printf("%s\n", config.signer.signupError.message.c_str());
  // }
  signupOK = true;
  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);
  Firebase.begin(&config, &auth);
}

void loop(){
  if (Firebase.isTokenExpired()){
    Firebase.refreshToken(&config);
    Serial.println("Refresh token");
  }

  if(Firebase.ready() && (millis() - sendDataPrevMillis > 5000 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();
    count++;
    

    if(Firebase.RTDB.setInt(&fbdo, uid+"/Monitor/data", airtemp+1)){
      Serial.println(); Serial.print(airtemp);
      Serial.print("- successfully saved to: " + fbdo.dataPath());
      Serial.println("("+ fbdo.dataType() +")");
    }else{
      Serial.println("FAILED: " + fbdo.errorReason());
    } 

    // //Firestore Write DATA
    // Serial.print("Batch write documents... ");

    // // The dyamic array of write object firebase_firestore_document_write_t.
    // std::vector<struct firebase_firestore_document_write_t> writes;

    // // A write object that will be written to the document.
    // struct firebase_firestore_document_write_t update_write;

    // // Set the write object write operation type.
    // // firebase_firestore_document_write_type_update,
    // // firebase_firestore_document_write_type_delete,
    // // firebase_firestore_document_write_type_transform
    // update_write.type = firebase_firestore_document_write_type_update;

    // // Set the document content to write (transform)
    // FirebaseJson content;
    // String documentPath = "test_collection/test_document_map_value";
    // String documentPath2 = "test_collection/test_document_timestamp";

    // content.set("fields/myMap/mapValue/fields/key" + String(count) + "/mapValue/fields/name/stringValue", "value" + String(count));
    // content.set("fields/myMap/mapValue/fields/key" + String(count) + "/mapValue/fields/count/stringValue", String(count));
    // // Set the update document content
    // update_write.update_document_content = content.raw();
    // update_write.update_masks = "myMap.key" + String(count);

    // // Set the update document path
    // update_write.update_document_path = documentPath.c_str();

    // // Add a write object to a write array.
    // writes.push_back(update_write);

    // // A write object that will be written to the document.
    // struct firebase_firestore_document_write_t transform_write;

    // // Set the write object write operation type.
    // // firebase_firestore_document_write_type_update,
    // // firebase_firestore_document_write_type_delete,
    // // firebase_firestore_document_write_type_transform
    // transform_write.type = firebase_firestore_document_write_type_transform;
    // // Set the document path of document to write (transform)
    // transform_write.document_transform.transform_document_path = documentPath2;
    // // Set a transformation of a field of the document.
    // struct firebase_firestore_document_write_field_transforms_t field_transforms;
    // // Set field path to write.
    // field_transforms.fieldPath = "myTime";
    // // Set the transformation type.
    // field_transforms.transform_type = firebase_firestore_transform_type_set_to_server_value;
    // // Set the transformation content, server value for this case.
    // // See https://firebase.google.com/docs/firestore/reference/rest/v1/Write#servervalue
    // field_transforms.transform_content = "REQUEST_TIME"; // set timestamp to timestamp field
    // // Add a field transformation object to a write object.
    // transform_write.document_transform.field_transforms.push_back(field_transforms);

    // // Add a write object to a write array.
    // writes.push_back(transform_write);
    // //if (Firebase.Firestore.batchWriteDocuments(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, writes /* dynamic array of firebase_firestore_document_write_t */, nullptr /* labels */))
    // if (Firebase.Firestore.batchWriteDocuments(&fbdo, FIREBASE_PROJECT_ID, uid+"/report" , writes , nullptr ))
    //   Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
    // else
    //   Serial.println("fs ERROR: "+fbdo.errorReason());
    String documentPath = "user/"+uid+"/report//";

    FirebaseJson content;

    double temp = 2.0;
    double humi = 1.0;

    content.set("fields/temperature/doubleValue", String(temp).c_str());
    content.set("fields/humidity/doubleValue", String(humi).c_str());

    if(Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(), "temperature,humidity")){
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



    
    
}

  //airtemp += 1;



