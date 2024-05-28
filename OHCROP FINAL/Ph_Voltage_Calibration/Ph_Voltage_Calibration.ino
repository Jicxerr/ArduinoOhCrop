int pH_Value;
float Voltage;

int RelayPin2 = 7;
int RelayPin1 = 4;

void setup() {
  Serial.begin(9600);
  pinMode(pH_Value, INPUT);

 //pinMode(RelayPin2, OUTPUT);//Relay
 //pinMode(RelayPin1, OUTPUT);//Relay
}

void loop() {
  //digitalWrite(RelayPin2, LOW);
  //digitalWrite(RelayPin1, LOW);
  pH_Value = analogRead(A2);
  Voltage = pH_Value * (5.0 / 1023.0);
  Serial.println(Voltage);
  delay(500);

}
