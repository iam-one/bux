#include <WiFi.h>
#include <PubSubClient.h>

// choose device type (A, B)
#define A
//#define B

// choose test type (DEV, PROD)
//#define DEV
#define PROD

// Declare topic constant
#ifdef A
  const char* CLIENT_NAME = "ESP32-A";
  const char* PRESS_TOPIC = "A/press";
  const char* SCORE_TOPIC = "A/score";
  const char* READY_TOPIC = "A/ready";
  const char* BLINK_TOPIC = "A/blink";
#endif
#ifdef B
  const char* CLIENT_NAME = "ESP32-B";
  const char* PRESS_TOPIC = "B/press";
  const char* SCORE_TOPIC = "B/score";
  const char* READY_TOPIC = "B/ready";
  const char* BLINK_TOPIC = "B/blink";
#endif

// Declare network info
#ifdef DEV
  const char* SSID = "G199_IoT_2.4G";
  const char* PASSWORD = "smart10_199@";
  const char* MQTT_SERVER = "10.244.104.50";
#endif
#ifdef PROD
  const char* SSID = "2-10_festival";
  const char* PASSWORD = "Y0uwou1d!ntgue$$th1s";
  const char* MQTT_SERVER = "192.168.4.1";
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
  Serial.print("Connecting to ");
  Serial.println(SSID);

  WiFi.begin(SSID, PASSWORD);

  Serial.print("WiFi Status: ");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(WiFi.status());
    Serial.print(" / ");
  }
  Serial.println("Connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Update on subscribed topic: ");
  Serial.print(topic);
  Serial.print("=");
  String buffer;

  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    buffer += (char)message[i];
  }
  Serial.println();

  // subcribe {device}/blink
  if (String(topic) == BLINK_TOPIC) {
    Serial.print("Published: ");
    Serial.print(BLINK_TOPIC);
    Serial.print(" = ");

    if(buffer == "true") isBlink = true;
    else if(buffer == "false") isBlink = false;
    Serial.println(isBlink);
  }

  // subcribe {device}/score
  if (String(topic) == SCORE_TOPIC) {
    Serial.print("Published: ");
    Serial.print(SCORE_TOPIC);
    Serial.print("=");

    if (buffer == "0") score = 0;
    else if (buffer == "1") score = 1;
    else if (buffer == "2") score = 2;
    else if (buffer == "3") score = 3;
    else {
      Serial.print("Exception: ");
      Serial.print(SCORE_TOPIC);
      Serial.print(" is invalid (not within 0~3), value = ");
      Serial.println(buffer);
    }
    Serial.println(score);
  }
}

void connectMQTT() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection... ");
    if (client.connect(CLIENT_NAME)) {
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

uint32_t next = 0;
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
  connectMQTT();
  client.loop();
  if (WiFi.status() != WL_CONNECTED) {
    setupWifi();
  }

  // publish press
  if (millis() > next) {
    client.publish(PRESS_TOPIC,digitalRead(buttonPin) ? "true" : "false");
    next = millis() + 100;
  }

  // blink while avaliable
  if (millis()-tick > 1000/pow(2, score)){
    if (ledState == false && isBlink){
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