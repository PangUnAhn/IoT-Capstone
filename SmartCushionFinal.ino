/*
  AWS IoT WiFi

  This sketch securely connects to an AWS IoT using MQTT over WiFi.
  It uses a private key stored in the ATECC508A and a public
  certificate for SSL/TLS authetication.

  It publishes a message every 5 seconds to arduino/outgoing
  topic and subscribes to messages on the arduino/incoming
  topic.

  The circuit:
  - Arduino MKR WiFi 1010 or MKR1000

  The following tutorial on Arduino Project Hub can be used
  to setup your AWS account and the MKR board:

  https://create.arduino.cc/projecthub/132016/securely-connecting-an-arduino-mkr-wifi-1010-to-aws-iot-core-a9f365

  This example code is in the public domain.
*/

#include <ArduinoBearSSL.h>
#include <ArduinoECCX08.h>
#include <ArduinoMqttClient.h>
#include <WiFiNINA.h> // change to #include <WiFi101.h> for MKR1000

#include <ArduinoJson.h>

#include "arduino_secrets.h"

// // #include "DHT.h"
// // #define DHTPIN 2     // Digital pin connected to the DHT sensor
// // #define DHTTYPE DHT11   // DHT 11
// // DHT dht(DHTPIN, DHTTYPE);

// #define LED_1_PIN 5

#include <ArduinoJson.h>
// #include "Led.h"

/////// Enter your sensitive data in arduino_secrets.h
const char ssid[]        = SECRET_SSID;
const char pass[]        = SECRET_PASS;
const char broker[]      = SECRET_BROKER;
const char* certificate  = SECRET_CERTIFICATE;

WiFiClient    wifiClient;            // Used for the TCP socket connection
BearSSLClient sslClient(wifiClient); // Used for SSL/TLS connection, integrates with ECC508
MqttClient    mqttClient(sslClient);

unsigned long lastMillis = 0;

// Led led1(LED_1_PIN);

int S0  = 5;
int S1  = 4;
int S2  = 3;
int S3  = 2;
 
//  mux ic selection pins. This shield uses 2 mux ic.
int En0 = 7;  //  Low enabled
int En1 = 6;  //  Low enabled
 
int controlPin[] = {S0,S1,S2,S3,En0,En1}; // same to {5, 4, 3, 2, 7, 6}
 
 
// adc pin, read sensor value. 
int ADC_pin = A3; // Arduino ProMicro : A3, Arduino NANO : A7
 
//  adc data buffer
const int NUM_OF_CH = 32;
int sensor_data[NUM_OF_CH];
 

void setup() {
   pinMode(En0, OUTPUT);
  pinMode(En1, OUTPUT);
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT); 

  Serial.begin(115200);
  while (!Serial);

  // dht.begin();

  if (!ECCX08.begin()) {
    Serial.println("No ECCX08 present!");
    while (1);
  }

  // Set a callback to get the current time
  // used to validate the servers certificate
  ArduinoBearSSL.onGetTime(getTime);

  // Set the ECCX08 slot to use for the private key
  // and the accompanying public certificate for it
  sslClient.setEccSlot(0, certificate);

  // Optional, set the client id used for MQTT,
  // each device that is connected to the broker
  // must have a unique client id. The MQTTClient will generate
  // a client id for you based on the millis() value if not set
  //
  // mqttClient.setId("clientId");

  // Set the message callback, this function is
  // called when the MQTTClient receives a message
  // mqttClient.onMessage(onMessageReceived);
}

void loop() {

 // Read ADC value. Scan 32 channels of shield.
  for(int ch = 0 ; ch < NUM_OF_CH ; ch++){ 
    sensor_data[ch] = readMux(ch);
  }

if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  if (!mqttClient.connected()) {
    // MQTT client is disconnected, connect
    connectMQTT();
  }

  // poll for new MQTT messages and send keep alives
  mqttClient.poll();

  // publish a message roughly every 5 seconds.
  if (millis() - lastMillis > 5000) {
    lastMillis = millis();
    char payload[512];
    getDeviceStatus(payload, sizeof(payload));

    // Print payload for debugging
    Serial.println(payload);
    sendMessage(payload);
  }
}

unsigned long getTime() {
  // get the current time from the WiFi module  
  return WiFi.getTime();
}

void connectWiFi() {
  Serial.print("Attempting to connect to SSID: ");
  Serial.print(ssid);
  Serial.print(" ");

  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    // failed, retry
    Serial.print(".");
    delay(5000);
  }
  Serial.println();

  Serial.println("You're connected to the network");
  Serial.println();
}

void connectMQTT() {
  Serial.print("Attempting to MQTT broker: ");
  Serial.print(broker);
  Serial.println(" ");

  while (!mqttClient.connect(broker, 8883)) {
    // failed, retry
    Serial.print(".");
    delay(5000);
  }
  Serial.println();

  Serial.println("You're connected to the MQTT broker");
  Serial.println();

  // subscribe to a topic
  mqttClient.subscribe("$aws/things/MyMKRWiFi1010/shadow/update/delta");
}

void getDeviceStatus(char* payload, size_t payloadSize) {
    // Create a JSON document
    StaticJsonDocument<512> doc; // Adjust size based on expected data size
    JsonObject state = doc.createNestedObject("state");
    JsonObject reported = state.createNestedObject("reported");
    JsonArray sensorArray = reported.createNestedArray("sensor_data");

    // Add collected sensor data to the JSON array
    for (int ch = 0; ch < NUM_OF_CH; ch++) {
        sensorArray.add(sensor_data[ch]);
    }

    // Serialize JSON document to string
    serializeJson(doc, payload, payloadSize);
}

void sendMessage(char* payload) {
  char TOPIC_NAME[]= "$aws/things/MyMKRWiFi1010/shadow/update";
  
  Serial.print("Publishing send message:");
  Serial.println(payload);
  mqttClient.beginMessage(TOPIC_NAME);
  mqttClient.print(payload);
  mqttClient.endMessage();
}

int readMux(int channel) {
  int muxChannel[NUM_OF_CH][6] = {
    //  {S0, S1, S2, S3, En0, En1}
    {0,0,0,0,0,1}, //channel 0
    {0,0,0,1,0,1}, //channel 1
    {0,0,1,0,0,1}, //channel 2
    {0,0,1,1,0,1}, //channel 3
    {0,1,0,0,0,1}, //channel 4
    {0,1,0,1,0,1}, //channel 5
    {0,1,1,0,0,1}, //channel 6
    {0,1,1,1,0,1}, //channel 7
    {1,0,0,0,0,1}, //channel 8
    {1,0,0,1,0,1}, //channel 9
    {1,0,1,0,0,1}, //channel 10
    {1,0,1,1,0,1}, //channel 11
    {1,1,0,0,0,1}, //channel 12
    {1,1,0,1,0,1}, //channel 13
    {1,1,1,0,0,1}, //channel 14
    {1,1,1,1,0,1}, //channel 15
    {0,0,0,0,1,0}, //channel 16
    {0,0,0,1,1,0}, //channel 17
    {0,0,1,0,1,0}, //channel 18
    {0,0,1,1,1,0}, //channel 19
    {0,1,0,0,1,0}, //channel 20
    {0,1,0,1,1,0}, //channel 21
    {0,1,1,0,1,0}, //channel 22
    {0,1,1,1,1,0}, //channel 23
    {1,0,0,0,1,0}, //channel 24
    {1,0,0,1,1,0}, //channel 25
    {1,0,1,0,1,0}, //channel 26
    {1,0,1,1,1,0}, //channel 27
    {1,1,0,0,1,0}, //channel 28
    {1,1,0,1,1,0}, //channel 29
    {1,1,1,0,1,0}, //channel 30
    {1,1,1,1,1,0}  //channel 31
  };

  // Config 6 digital out pins of 2 mux ICs.
  for(int i = 0; i < 6; i++){
    digitalWrite(controlPin[i], muxChannel[channel][i]);
  }

  // Read sensor value
  int adc_value = analogRead(ADC_pin);
  return adc_value;
}


// void getDeviceStatus(char* payload) {
//   // Read temperature as Celsius (the default)
//   float t = dht.readTemperature();

//   // Read led status
//   const char* led = (led1.getState() == LED_ON)? "ON" : "OFF";

//   // make payload for the device update topic ($aws/things/MyMKRWiFi1010/shadow/update)
//   sprintf(payload,"{\"state\":{\"reported\":{\"temperature\":\"%0.2f\",\"LED\":\"%s\"}}}",t,led);
// }

// void sendMessage(char* payload) {
//   char TOPIC_NAME[]= "$aws/things/MyMKRWiFi1010/shadow/update";
  
//   Serial.print("Publishing send message:");
//   Serial.println(payload);
//   mqttClient.beginMessage(TOPIC_NAME);
//   mqttClient.print(payload);
//   mqttClient.endMessage();
// }


// void onMessageReceived(int messageSize) {
//   // we received a message, print out the topic and contents
//   Serial.print("Received a message with topic '");
//   Serial.print(mqttClient.messageTopic());
//   Serial.print("', length ");
//   Serial.print(messageSize);
//   Serial.println(" bytes:");

//   // store the message received to the buffer
//   char buffer[512] ;
//   int count=0;
//   while (mqttClient.available()) {
//      buffer[count++] = (char)mqttClient.read();
//   }
//   buffer[count]='\0'; // 버퍼의 마지막에 null 캐릭터 삽입
//   Serial.println(buffer);
//   Serial.println();

//   // JSon 형식의 문자열인 buffer를 파싱하여 필요한 값을 얻어옴.
//   // 디바이스가 구독한 토픽이 $aws/things/MyMKRWiFi1010/shadow/update/delta 이므로,
//   // JSon 문자열 형식은 다음과 같다.
//   // {
//   //    "version":391,
//   //    "timestamp":1572784097,
//   //    "state":{
//   //        "LED":"ON"
//   //    },
//   //    "metadata":{
//   //        "LED":{
//   //          "timestamp":15727840
//   //         }
//   //    }
//   // }
//   //
//   DynamicJsonDocument doc(1024);
//   deserializeJson(doc, buffer);
//   JsonObject root = doc.as<JsonObject>();
//   JsonObject state = root["state"];
//   const char* led = state["LED"];
//   Serial.println(led);
  
//   char payload[512];
  
//   if (strcmp(led,"ON")==0) {
//     led1.on();
//     sprintf(payload,"{\"state\":{\"reported\":{\"LED\":\"%s\"}}}","ON");
//     sendMessage(payload);
    
//   } else if (strcmp(led,"OFF")==0) {
//     led1.off();
//     sprintf(payload,"{\"state\":{\"reported\":{\"LED\":\"%s\"}}}","OFF");
//     sendMessage(payload);
//   }
 
//}
