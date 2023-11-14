
#include <Wire.h>

#define trig 12
#define echo 14


//Enter your tank max value(CM)
int MaxLevel = 9;

int Level1 = (MaxLevel * 80) / 100;
int Level2 = (MaxLevel * 65) / 100;
int Level3 = (MaxLevel * 55) / 100;
int Level4 = (MaxLevel * 35) / 100;

void setup() {
  // Debug console
  Serial.begin(115200);

  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);

  Serial.println("Loading");
  delay(4000);

}

//Get the ultrasonic sensor values
void ultrasonic() {
  digitalWrite(trig, LOW);
  delayMicroseconds(4);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long t = pulseIn(echo, HIGH);
  int distance = t / 29 / 2;

  Serial.println(distance);
  /*
  int blynkDistance = (distance - MaxLevel) * -1;
  if (distance <= MaxLevel) {
    Blynk.virtualWrite(V0, blynkDistance);
  } else {
    Blynk.virtualWrite(V0, 0);
  }
  lcd.setCursor(0, 0);
  lcd.print("WLevel:");*/

  if (Level1 <= distance) {
    Serial.println("Level 1");
  } else if (Level2 <= distance && Level1 > distance) {
    Serial.println("Level 2");
  } else if (Level3 <= distance && Level2 > distance) {
    Serial.println("Level 3");
  } else if (Level4 <= distance && Level3 > distance) {
    Serial.println("Level 4");
  }
}

//Get the button value
/*
BLYNK_WRITE(V1) {
  bool Relay = param.asInt();
  if (Relay == 1) {
    digitalWrite(relay, LOW);
    lcd.setCursor(0, 1);
    lcd.print("Motor is ON ");
  } else {
    digitalWrite(relay, HIGH);
    lcd.setCursor(0, 1);
    lcd.print("Motor is OFF");
  }
}*/

void loop() {
  ultrasonic();
  delay(500);
  //Blynk.run();//Run the Blynk library
}