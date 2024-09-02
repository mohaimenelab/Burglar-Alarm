// Library Declaration
#include <WiFi.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>
#include <string.h>
#include <LiquidCrystal.h>

// WiFi Variables
const char *ssid = "AIRNET";
const char *password = "wappassbtcl";
int port = 3000;  // Server Opening PORT

WiFiServer server(port);

IPAddress clientIP;
#define MAX_CLIENTS 2 // Maximum number of clients
IPAddress clients[MAX_CLIENTS]; // Declare Clients Array
WiFiClient newClients[MAX_CLIENTS];
IPAddress tempClients[MAX_CLIENTS];

unsigned long lastClientDataTime[MAX_CLIENTS]; //Declare Client Connected Array
const unsigned long CLIENT_TIMEOUT = 10000; // in milliseconds

IPAddress staticIP(192, 168, 102, 90);  // Static IP address to assign to ESP32
IPAddress gateway(192, 168, 100, 4);     // Gateway IP address
IPAddress subnet(255, 255, 255, 0);    // Subnet mask

//HardwareSerial Declaration
SoftwareSerial sim800l(16, 17); // RX = 16, TX = 17 So sim800l(16,17)
LiquidCrystal LCD(4, 2, 21, 19, 18, 5);

//Variables
String clientData;
String nodeStatus;
String nodeNumber;

String common_number = "01713043520";

byte buffer[4];
// char mobileNumber[11]; // variable to hold mobile number
char mobileNumber1[20];
char mobileNumber2[20];

char incomingMessage[100]; // buffer to hold incoming SMS message
int msgIndex = 0; // index for incomingMessage buffer

bool isRoomOneTheft;
bool isRoomTwoTheft;

bool isRoomOneSMSSent;
bool isRoomTwoSMSSent;

bool canSendWiFiWarning=false;


#define BUZZER_RELAY 23
#define NETWORK_FEEDBACK_LED 12
#define PIR_STATUS 33
#define BUTTON_PIN 13

bool isInIntruderTriggerMode;
bool isAllDeviceConnected;

void setup() {
  canSendWiFiWarning=false;
  delay(1000);
  InitializeEEPROM();
  InitializeLCD();
  Serial.begin(9600);
  InitializeGSM();
  // PushToGSM(mobileNumber1, "Device Started");
  // delay(3000);
  // PushToGSM(mobileNumber2, "Device Started");
  // delay(3000);
  // CallToGSM(mobileNumber);
  ConnectToWiFi();


  pinMode(BUZZER_RELAY, OUTPUT);
  pinMode(NETWORK_FEEDBACK_LED, OUTPUT);
  pinMode(PIR_STATUS, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  delay(500);
}

void InitializeEEPROM(){
  delay(500);
  EEPROM.begin(100);
  EEPROM.get(0*sizeof(char), mobileNumber1);
  delay(500);
  Serial.print("Mobile No: ");
  Serial.println(mobileNumber1);

  EEPROM.get(50*sizeof(char), mobileNumber2);
  delay(500);
  Serial.print("Mobile No: ");
  Serial.println(mobileNumber2);
}

void InitializeLCD(){
  LCD.begin(20, 4);
  LCD.setCursor(2,0);
  LCD.print("Aqualink BD LTD.");
}

void InitializeGSM(){
  LCD.setCursor(2,1);
  LCD.print("Initializing GSM");
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

void ConnectToWiFi(){
  WiFi.config(staticIP, gateway, subnet);  // Assign static IP address
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
 
  Serial.print("Connecting to WiFi");
  LCD.setCursor(1,1);
  LCD.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {   
    Serial.print(".");
    CheckForSMS();
    if(canSendWiFiWarning){
        canSendWiFiWarning = false;
        // EEPROM.get(0, mobileNumber);
        PushToGSM(mobileNumber1, "WiFi Disconnected");
        // EEPROM.get(15, mobileNumber);
        PushToGSM(mobileNumber2, "WiFi Disconnected");
    }
    delay(500);
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  
  server.begin();
  Serial.print("Open Telnet and connect to IP:");
  Serial.print(WiFi.localIP());
  Serial.print(" on port ");
  Serial.println(port);
  digitalWrite(NETWORK_FEEDBACK_LED, HIGH);
  LCD.setCursor(1,1);
  LCD.print(" Connected to WiFi");
  LCD.setCursor(0,2);
  LCD.print("Waiting For Clients.");
  delay(1000);
  canSendWiFiWarning = true;
}


void loop() {
  // Serial.print("One: ");
  // Serial.println(mobileNumber1);
  // delay(70);
  // Serial.print("Two: ");
  // Serial.println(mobileNumber2);
  // delay(70);
  try{
      ProcessOnClientConnected();
  }catch(...){
    Serial.println("Connection Error");
  }
  

  try{
    ReceiveDataFromClients();
  }catch(...){
    Serial.println("Receive Data Issue");
  }
  
  CheckForSMS();

  try{
    CheckClientTimeouts();
  }catch(...){
    Serial.println("Timeout Checking Issue");
  }
  
  try{
    OverrideAndTurnOffBuzzer();
  }catch(...){
    Serial.println("Input PULLUP issue");
  }
}

void OverrideAndTurnOffBuzzer(){
  if(digitalRead(BUTTON_PIN)==LOW){
    Serial.println("Button Pressed");
    ResetIntruderTriggerCallback();
  }
}

void ReceiveDataFromClients(){

  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (newClients[i] && newClients[i].connected()) {
      if (newClients[i].available()) {
        try{
            WiFiClient client = newClients[i];
            clientIP = client.remoteIP();
            client.read(buffer, sizeof(buffer));
            clientData = "";
            for(int i=0; i<sizeof(buffer); i++){
              clientData+=(char)buffer[i];
            }
            if(clientData!=""){
              OnReceiveServerOperations();
              // lastClientDataTime[i] = millis();
            }else{
              Serial.println("No Data From Client");
            }
        }catch(...){
          Serial.println("Data Read Error");
        }
      }
    }
    else {
      if (newClients[i]) {
        Serial.print("Client ");
        Serial.print(i);
        Serial.print(" disconnected: ");
        Serial.println(newClients[i].remoteIP());
        newClients[i].stop();
      }
    }
  }
  
}

void ProcessOnClientConnected() {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (!newClients[i]) {
      newClients[i] = server.available();
      newClients[i].setNoDelay(true);
      // Serial.print("New client: ");
      // Serial.println(newClients[i].remoteIP());

      clients[i] = newClients[i].remoteIP();
      lastClientDataTime[i] = millis();
      
      break;
    }
  }
}



void OnReceiveServerOperations(){
 // read data from the connected client
        // Serial.write(client.read());
        // clientData = String(client.readString());
        // Serial.print("Raw Data: ");        
        // Serial.println(clientData);
        nodeStatus = clientData.substring(0,1);
        nodeNumber = clientData.substring(1);
        Serial.print("Node Status: ");
        Serial.println(nodeStatus);
        Serial.print("Node Number: ");
        Serial.println(nodeNumber);
        
        LCD.setCursor(0,2);
        LCD.print("R-One: ");
        LCD.setCursor(10,2);
        LCD.print("R-Two: ");
        if(nodeNumber=="One"){
          clients[0] = clientIP;
          if(nodeStatus=="0" && !isRoomOneTheft){
            LCD.setCursor(7,2);
            LCD.print("S  ");
          }else{
            isRoomOneTheft = true;
            LCD.setCursor(7,2);
            LCD.print("D  ");
            if(!isRoomOneSMSSent){
              isRoomOneSMSSent = true;

              // EEPROM.get(0, mobileNumber);
              // delay(200);
              TriggerBuzzerRelay();
              PushToGSM(mobileNumber1, "Intruder Alert Room One");
              // CallToGSM(mobileNumber);
              isInIntruderTriggerMode = true;
            }
          }
        }
        if(nodeNumber=="Two"){
          clients[1] = clientIP;
          if(nodeStatus=="0" && !isRoomTwoTheft){
            LCD.setCursor(17,2);
            LCD.print("S  ");
          }else{
            isRoomTwoTheft = true;
            LCD.setCursor(17,2);
            LCD.print("D  ");
            if(!isRoomTwoSMSSent){
              isRoomTwoSMSSent = true;

              // EEPROM.get(15, mobileNumber);
              // delay(200);
              TriggerBuzzerRelay();
              PushToGSM(mobileNumber2, "Intruder Alert Room Two");
              // CallToGSM(mobileNumber);
              isInIntruderTriggerMode = true;
            }
          }
        }

        // Identify the client using its IP address
        
        int clientIndex = -1;
        for (int i = 0; i < MAX_CLIENTS; i++) {
          if (clients[i] == clientIP) {
            clientIndex = i;
            break;
          }
        }
        if (clientIndex == -1) {
          Serial.println("Unknown client");
        } else {
          Serial.print("Client ");
          Serial.print(clientIndex);
          Serial.print(" IP: ");
          Serial.println(clientIP);
          lastClientDataTime[clientIndex] = millis(); 
        }

        if(nodeStatus.equals("1") && !isInIntruderTriggerMode){
          
          
        }
        else if(nodeStatus.equals("0")){
          Serial.println();
        //  ResetIntruderTriggerCallback(); // Currently Set Here for Testing Purpose
        }
}

void CheckClientTimeouts() {

  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (newClients[i] && newClients[i].connected()) {
      if (millis() - lastClientDataTime[i] > CLIENT_TIMEOUT) {
        Serial.print("Client ");
        Serial.print(i);
        Serial.print(" timed out: ");
        Serial.println(newClients[i].remoteIP());
        // for (int j = 0; j < MAX_CLIENTS; j++) {
        //   if(newClients[i].remoteIP()==clients[j]){
        //     clients[j] = (IPAddress)INADDR_NONE;
        //     Serial.println("Timeout Set");
        //     if(j==0){
        //       LCD.setCursor(0,3);
        //       LCD.print("One: DC");
        //     }
        //     if(j==1){
        //       LCD.setCursor(10,3);
        //       LCD.print("Two: DC");
        //     }
        //   }
        // }
        newClients[i].stop();
      }
    }
  }
  
//   unsigned long now = millis();
//   for (int i = 0; i < MAX_CLIENTS; i++) {
//     if (clients[i] != (IPAddress)INADDR_NONE && now - lastClientDataTime[i] > CLIENT_TIMEOUT) {
//       // client[i] has timed out
//       Serial.print("Client ");
//       Serial.print(i);
//       Serial.println(" has timed out");
// //      LCD.setCursor(0,3);
// //      LCD.print("Client "+String(i+1)+"DC");
//       clients[i] = (IPAddress)INADDR_NONE; // mark the client as disconnected
//     }
//   }

  // if(clients[0]==(IPAddress)INADDR_NONE){
  //   LCD.setCursor(0,3);
  //   LCD.print("One: DC");
  // }else{
  //   LCD.setCursor(0,3);
  //   LCD.print("One:  C");
  // }

  // if(clients[1]==(IPAddress)INADDR_NONE){
  //   LCD.setCursor(10,3);
  //   LCD.print("Two: DC");
  // }else{
  //   LCD.setCursor(10,3);
  //   LCD.print("Two:  C");
  // }
  
}


void ResetIntruderTriggerCallback(){
  isInIntruderTriggerMode = false;
  digitalWrite(BUZZER_RELAY, LOW);
  digitalWrite(PIR_STATUS, LOW);
  isRoomOneTheft = false;
  isRoomTwoTheft = false;
  isRoomOneSMSSent = false;
  isRoomTwoSMSSent = false;
}

void TriggerBuzzerRelay(){
  Serial.println("Buzzer Triggered");
  digitalWrite(PIR_STATUS, HIGH);
  digitalWrite(BUZZER_RELAY, HIGH);
}

void PushToGSM(String number, String message){
  // GSM AT Command
  /*sim800l.println("AT+CMGF=1"); // Set the SMS mode to text mode
  delay(100);
  sim800l.println("AT+CMGS=\""+number+"\""); // Set the phone number you want to send SMS to
  delay(100);
  sim800l.println(message); // Replace with the message you want to send
  delay(100);
  sim800l.write(26); // Send the CTRL+Z character to end the SMS message
  delay(100);*/
  
  sim800l.print("ATD");
  sim800l.print(number);
  sim800l.println(";"); // Replace with the message you want to send
  delay(20000);
  sim800l.print("ATH"); 
  delay(500);
  InitializeCommonNumber();
  //Serial.println("sending call to" + common_number);
  sim800l.print("ATD");
  sim800l.print(common_number);
  sim800l.println(";"); // Replace with the message you want to send
  delay(20000);
  sim800l.print("ATH"); 
  delay(100);
  //Serial.println("phone call done"); 
}

void CallToGSM(String number){
  
}

void CheckForSMS(){
   while(sim800l.available()){
    String data = sim800l.readStringUntil('\n');
    Serial.println(data);
    delay(500);
    String starter = data.substring(0,2);
    if(starter=="PH"){
      String index = data.substring(2,3);
      if(index=="1"){
        strcpy(mobileNumber1, data.substring(3).c_str()); // copy phone number to mobileNumber variable
        Serial.println(mobileNumber1);
        delay(1000);
        EEPROM.put(0*sizeof(char), mobileNumber1); // save phone number to EEPROM
        delay(100);
        EEPROM.commit();
        delay(100);
        Serial.println("Phone number saved: " + String(mobileNumber1));
        sim800l.println("AT+CMGD=1");
        delay(100);
      }else if(index=="2"){
        strcpy(mobileNumber2, data.substring(3).c_str()); // copy phone number to mobileNumber variable
        Serial.println(mobileNumber2);
        delay(1000);
        EEPROM.put(50*sizeof(char), mobileNumber2); // save phone number to EEPROM
        delay(100);
        EEPROM.commit();
        delay(100);
        Serial.println("Phone number saved: " + String(mobileNumber2));
        sim800l.println("AT+CMGD=1");
        delay(100);
      }else{
        Serial.println("Wrong Index");
      }
      
    }else{
      Serial.println("SMS received: "+ data);
      
    }
    
  }
}
