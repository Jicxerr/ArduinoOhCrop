#define PHSensorPin A2
float calibration_value = 21.34 - .4  ;//
int phval = 0; 
unsigned long int avgval; 
int buffer_arr[10],temp;

float ph_Value;
int RelayPin2 = 7;
int RelayPin1 = 4;


void setup() {
  Serial.begin(9600);
   pinMode(RelayPin2, OUTPUT);//Relay
   pinMode(RelayPin1, OUTPUT);//Relay
}

void loop() {
  digitalWrite(RelayPin2, LOW);
  digitalWrite(RelayPin1, LOW);

  for(int i=0;i<10;i++) { 
    buffer_arr[i]=analogRead(PHSensorPin);
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
 float volt=(float)avgval*5.0/1024/6; 
  ph_Value = -5.70 * volt + calibration_value;

 Serial.print("pH Val: ");
 Serial.println(ph_Value);
 delay(1000);
}
