#include <Arduino.h>

#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <ArduinoJson.h>

DHT_Unified dht(27, DHT22);

const char* ssid = "GO-FT";
const char* password = "GOtech!!";
const char* mqtt_server = "192.168.3.2";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
String JSONmessage;

const int ledPin = 14;

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }

  if (String(topic) == "esp32/output") {
    Serial.print("Changing output to ");
    if(messageTemp == "on"){
      Serial.println("on");
      digitalWrite(ledPin, HIGH);
    }
    else if(messageTemp == "off"){
      Serial.println("off");
      digitalWrite(ledPin, LOW);
    }
  }
}

void setup() {
  Serial.begin(115200);

  delay(10);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  dht.begin();
  sensor_t sensor;

  pinMode(ledPin, OUTPUT);
}

void loop() {
    while (!client.connected()) {
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      client.subscribe("esp32/output");
    } else {
      Serial.print("failed");
      delay(5000);
    }
  }

  client.loop();

  long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;
    StaticJsonDocument<200> doc;

    int co2 = analogRead(25);

    sensors_event_t event;
    dht.temperature().getEvent(&event);
    doc["temperature"] = event.temperature;
    dht.humidity().getEvent(&event);
    doc["humidity"] = event.relative_humidity;
    doc["co2"] = co2;

    JSONmessage = "";
    serializeJson(doc, JSONmessage);
    JSONmessage.toCharArray(msg, JSONmessage.length() + 1);
    client.publish("IOT-0/values", msg);
  }
}