#include <SoftwareSerial.h>
String number        = "01628870079"; 
String common_number = "01778973689";

SoftwareSerial sim800l(8, 9); // RX = 8, TX = 9 

void setup() {
 Serial.begin(9600);
 InitializeGSM();
 PushToGSM();
}

void loop() {
 
}

void InitializeGSM(){
  sim800l.begin(9600); // For SIM800L
  delay(1000);
  sim800l.println("AT+CMGF=1"); // Set the SMS mode to text mode
  delay(100);
  sim800l.println("AT+CNMI=2,2,0,0,0"); // Configure SIM800L to automatically notify whenever a new SMS is received
  delay(2000);
}

void InitializeCommonNumber(){
  sim800l.begin(9600); // For SIM800L
  delay(1000);
  sim800l.println("AT+CMGF=1"); // Set the SMS mode to text mode
  delay(100);
  sim800l.println("AT+CNMI=2,2,0,0,0"); // Configure SIM800L to automatically notify whenever a new SMS is received
  delay(2000);
}

void PushToGSM(){
  Serial.println("sending call to" + number);
  sim800l.print("ATD");
  sim800l.print(number);
  sim800l.println(";"); // Replace with the message you want to send
  delay(20000);
  sim800l.print("ATH"); 
  delay(500);
  InitializeCommonNumber();
  Serial.println("sending call to" + common_number);
  sim800l.print("ATD");
  sim800l.print(common_number);
  sim800l.println(";"); // Replace with the message you want to send
  delay(20000);
  sim800l.print("ATH"); 
  delay(100);
  Serial.println("phone call done");
}
