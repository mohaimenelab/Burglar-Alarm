//Library Declaration
#include <WiFi.h>

#ifndef STASSID
#define STASSID "AIRNET"
#define STAPSK "wappassbtcl"
#endif

//WiFi Variables
const char* ssid = STASSID;
const char* password = STAPSK;

//TCP-IP Socketing
const char* host = "192.168.102.90"; //Server ESP IP
const uint16_t port = 3000; // Server Opening PORT

//Variables

#define PIR_PIN 14 //PIR Program Pin
#define PIR_LED_STATUS 26 // PIR Feedback Pin
#define Network_Feedback_LED 27

byte bufferY[] = {0x31, 0x4F, 0x6E, 0x65};
byte bufferN[] = {0x30, 0x4F, 0x6E, 0x65};

//byte bufferY[] = {0x31, 0x54, 0x77, 0x6F};
//byte bufferN[] = {0x30, 0x54, 0x77, 0x6F};



int pirStatus;
int retryToConnect = 0;
int pulse_counter = 0;
float total_up_time = 0;
float total_down_time = 0;
unsigned long last_motion_reset = millis();
float last_up_time = 0;
float last_down_time = 0;
int last_state = 0;
//Class Creation
WiFiClient client;


void setup() {
  Serial.begin(9600);
  ConnectToWiFi();

  //PinMode Declaration
  pinMode(PIR_PIN, INPUT_PULLUP);
  pinMode(PIR_LED_STATUS, OUTPUT);
  pinMode(Network_Feedback_LED, OUTPUT);
  
  Serial.println("Sensor Calibration Ongoing...");
  delay(3000);
}


void loop() {

  pulse_count();
  motion_reset();
  delay(10);
}

/*

  Method Regions

*/

void pulse_count() {

  pirStatus = digitalRead(PIR_PIN);
  //Serial.print("Pin Status: ");
  //Serial.println(pirStatus);
  float current_time = millis() / 1000;
  if (pirStatus == HIGH && last_state == LOW) {

    digitalWrite(Network_Feedback_LED, HIGH);
    //total_up_time=total_up_time+(current_time-last_up_time);
    total_down_time = total_down_time + (current_time - last_down_time);
    Serial.print("Last down interval");
    Serial.println(current_time - last_down_time);
    last_up_time = current_time;
    pulse_counter++;
    if (pulse_counter == 1) {
      last_motion_reset = millis();
    }
    last_state = HIGH;
  }
  else if (pirStatus == LOW && last_state == HIGH) {

    digitalWrite(Network_Feedback_LED, LOW);

    //total_down_time=total_down_time+(current_time-last_down_time);
    total_up_time = total_up_time + (current_time - last_up_time);
    Serial.print("Last up interval");
    Serial.println(current_time - last_up_time);
    last_down_time = current_time;
    last_state = LOW;

    send_data(false);

  }
  else if (pirStatus == LOW ) {
    last_state = LOW;
  }
  else if (pirStatus == HIGH ) {
    last_state = HIGH;
  }

}

void motion_reset() {

  if (millis() - last_motion_reset > 8000) {
    Serial.print("Motion detected. Count:");
    Serial.println(pulse_counter);
    Serial.print("total up time:");
    Serial.println(total_up_time);
    Serial.print("total down time:");
    Serial.println(total_down_time);

    if (total_up_time > 3) {
      Serial.println("Motion data Sending...");
      send_data(true);
    }
    else if (pulse_counter >= 2 && total_up_time >= 1) {
      Serial.println("Motion data Sending...");
      send_data(true);
    }
    else {
      send_data(false);
    }
    Serial.println("motion reset");
    total_up_time = 0;
    total_down_time = 0;
    pulse_counter = 0;
    last_motion_reset = millis();
    last_up_time = millis() / 1000;
    last_down_time = millis() / 1000;
  }
}

void send_data(bool state) {

  if (client.connect(host, port)) {
    if (state == true) {
      client.write(bufferY, sizeof(bufferY));
      Serial.println("Motion data sent");
    }
    else if (state == false) {
      client.write(bufferN, sizeof(bufferN));
    }
    client.stop();
  }


}



void RetryConnection() {

  if (WiFi.status() != WL_CONNECTED) {
    ConnectToWiFi();
  }
}

//Connecting WiFi
void ConnectToWiFi() {

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.println();
  Serial.println();
  Serial.print("Connecting To WiFi...");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.println("WiFi Connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  digitalWrite(Network_Feedback_LED, HIGH);

}
