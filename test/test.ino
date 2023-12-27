#include <WiFi.h>
#include <PubSubClient.h>
#include "env.h"

// choose device type (A, B)
#define A
//#define B

// Declare topic constant
#ifdef A
  const char* PRESS_TOPIC = "A/press";
  const char* SCORE_TOPIC = "A/score";
  const char* READY_TOPIC = "A/ready";
  const char* BLINK_TOPIC = "A/blink";
#endif
#ifdef B
  const char* PRESS_TOPIC = "B/press";
  const char* SCORE_TOPIC = "B/score";
  const char* READY_TOPIC = "B/ready";
  const char* BLINK_TOPIC = "B/blink";
#endif

// Declare I/O Pin
const uint8_t buttonPin = 12;
const uint8_t ledPin = 14;

uint8_t score = 0; // variable for blink interval
uint32_t tick = 0;
bool isBlink = false; // subcribed from MQTT

bool ledState = false; // variable to prevent useless output in blink

WiFiClient espClient;
PubSubClient client(espClient);

void setupWifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(SSID);

  WiFi.begin(SSID, PASSWORD);

  Serial.print("WiFi Status: ");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(WiFi.status());
    Serial.print(" / ");
  }

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // subcribe {device}/blink
  if (String(topic) == BLINK_TOPIC) {
    Serial.print("Published: ");
    Serial.print(BLINK_TOPIC);
    Serial.print(" = ");

    if(messageTemp == "true"){
      Serial.println("true");
      isBlink = true;
    }
    else if(messageTemp == "false"){
      Serial.println("false");
      isBlink = false;
    }
  }

  // subcribe {device}/score
  if (String(topic) == SCORE_TOPIC) {
    Serial.print("Published: ");
    Serial.print(SCORE_TOPIC);
    Serial.print("=");

    if (messageTemp == "0") {
      score = 0;
      Serial.println("0");
    }
    else if (messageTemp == "1") {
      score = 1;
      Serial.println("1");
    }
    else if (messageTemp == "2") {
      score = 2;
      Serial.println("2");
    }
    else if (messageTemp == "3") {
      score = 3;
      Serial.println("3");
    }
    else {
      Serial.print("Exception: ");
      Serial.print(SCORE_TOPIC);
      Serial.print(" is invalid (not within 0~3), value = ");
      Serial.println(messageTemp);
    }
  }
}

void connectMQTT() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection... ");
    if (client.connect("ESP32Client")) {
      Serial.println("Connected");
      client.publish(READY_TOPIC, "true");
      client.publish(PRESS_TOPIC, "false");
      client.subscribe(BLINK_TOPIC);
      client.subscribe(SCORE_TOPIC);
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(". Try again within 3s...");
      delay(3000);
    }
  }
}

void setup(){
  Serial.begin(115200);
  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);

  setupWifi();

  client.setServer(MQTT_SERVER, 1883);
  client.setCallback(callback);
  
  connectMQTT();

  // publish device status
  client.publish(READY_TOPIC, "true");
  client.publish(PRESS_TOPIC, "false");
}

void loop(){
  if (client.connect("ESP32Client")) {
    connectMQTT();
  }
  client.loop();

  if (isBlink == true){
    // publish press
    if (digitalRead(buttonPin)) {
      client.publish(PRESS_TOPIC, "true");

      // prevent press
      Serial.println("Pressed. Solid 5s...");
      digitalWrite(ledPin, HIGH);
      delay(5000);
      digitalWrite(ledPin, LOW);
      client.publish(PRESS_TOPIC, "false");
      Serial.println("Switch is available.");
    }

    // blink while avaliable
    if (millis()-tick > 1000/pow(2, score)){
      if (ledState == false){
        Serial.println("blink: on");
        digitalWrite(ledPin, HIGH);
        ledState = true;
      }
    }
    if (millis()-tick > 2000/pow(2, score)){
      tick = millis();
      digitalWrite(ledPin, LOW);
      Serial.println("blink: off");
      ledState = false;
    }
  }
}