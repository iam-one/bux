#include <WiFi.h>
#include <PubSubClient.h>

#define A
//#define B
#define DEV
//#define PROD

#ifdef A
  const char* pubTopic = "state/score/A";
#endif
#ifdef B
  const char* pubTopic = "state/score/B";
#endif
#ifdef DEV
  const char* ssid = "G199_IoT_2.4G";
  const char* password = "smart10_199@";
  const char* mqttServer = "10.244.104.50";
#endif
#ifdef PROD
  const char* ssid = "2-10_festival";
  const char* password = "Y0uwou1d!ntgue$$th1s";
  const char* mqttServer = "192.168.4.1";
#endif

const uint8_t buttonPin = 12;
const uint8_t ledPin = 14;

uint8_t score = 0;
uint32_t currentTick = 0;
bool isReady = false;
bool isOn = false;

WiFiClient espClient;
PubSubClient client(espClient);

void setupWifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("WiFi Status: ");
    Serial.print(WiFi.status());
    Serial.println();
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

  if (String(topic) == "state/ready") {
    Serial.print("Published: state/ready=");

    if(messageTemp == "true"){
      Serial.print("true");
      isReady = true;
    }
    else if(messageTemp == "false"){
      Serial.print("false");
      isReady = false;
    }
  }

  if (String(topic) == pubTopic) {
    Serial.print("Published: ");
    Serial.print(pubTopic);
    Serial.print("=");

    if(messageTemp == "0"){
      Serial.print("0");
      score = 0;
    }
    else if(messageTemp == "1"){
      Serial.print("1");
      score = 1;
    }
    else if(messageTemp == "2"){
      Serial.print("2");
      score = 2;
    }
    else if(messageTemp == "3"){
      Serial.print("3");
      score = 3;
    }
  }
}

void connectMQTT() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection... ");
    if (client.connect("ESP32Client")) {
      Serial.println("Connected");
      client.subscribe("state/ready");
      client.subscribe(pubTopic);
    } else {
      Serial.print("failed, rc=");
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

  client.setServer(mqttServer, 1883);
  client.setCallback(callback);
  
  connectMQTT();

  client.publish(pubTopic, {"0"});
}

void loop(){
  if (client.connect("ESP32Client")) {
    connectMQTT();
  }
  client.loop();

  // get pushed
  if (isReady == true && digitalRead(buttonPin)){
    // ready initialize
    isReady = false;
    client.publish("state/ready", "false");

    // score publication
    if (blink == 0) {
      blink = 1;
      client.publish(pubTopic, {"1"});
    }
    else if (blink == 1) {
      blink = 2;
      client.publish(pubTopic, {"2"});
    }
    else if (blink == 2) {
      blink = 3;
      client.publish(pubTopic, {"3"});
    }
    else if (blink == 3) {
      blink = 0;
      client.publish(pubTopic, {"0"});
    }
  }

  // blink
  if (millis()-currentTick > 1000/pow(2, blink)){
    digitalWrite(ledPin, HIGH);
  }
  if (millis()-currentTick > 2000/pow(2, blink)){
    currentTick = millis();
    digitalWrite(ledPin, LOW);
  }
}