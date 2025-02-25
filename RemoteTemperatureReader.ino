#include <DHT.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define ssid F("")
#define password F("")
#define DHT22_PIN 13  // ESP32 pin GPIO13 connected to DHT22 sensor
#define LED 2
#define mqttPort 1883
#define mqttServer ""
#define mqttUser ""
#define mqttPassword ""
#define mqttSensorTopic "home/external/sensor"
unsigned long previousMillis = 0;
const long interval = 300000;  // Interval at which to read (5 minutes in milliseconds)


DHT dht22(DHT22_PIN, DHT22);
WiFiClient espClient;
PubSubClient client(espClient);
int errorCounter = 0;

void setup() {
  Serial.begin(921600);
  delay(200);
  Serial.println(F("Home assistant integrated temperature & humidity reader - Welcome!"));
  if (errorCounter > 5) {
    errorCounter = 0;
    Serial.println(F("Restored from error"));
  }
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  pinMode(LED, OUTPUT);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("no wifi"));
    delay(250);
  }
  client.setServer(mqttServer, mqttPort);
  client.setKeepAlive(60);
  connectMqtt();
  dht22.begin();
}

void connectMqtt() {
  while (!client.connected()) {
    String client_id = "esp32-client-";
    client_id += String(WiFi.macAddress());
    if (client.connect(client_id.c_str(), mqttUser, mqttPassword)) {
      Serial.println(F("Reconnected to MQTT broker"));
    } else {
      Serial.print(F("Failed to connect, state "));
      Serial.print(client.state());
      delay(5000);  // wait for 5 seconds before retrying
    }
  }
}

void readFromSensor() {
  digitalWrite(LED, HIGH);
  dht22.read();
  float h = dht22.readHumidity();
  float t = dht22.readTemperature();
  if (isnan(h) || isnan(t) || t > 60) {
    Serial.println(F("Failed to read from DHT sensor!"));
    delay(2000);
    errorCounter += 1;
    return;
  }
  if (t < 0) {
    t = 0 - (t + 3276.70);
  }
  if (WiFi.status() == WL_CONNECTED) {
    char payload[60];
    snprintf(payload, sizeof(payload), "{\"temperature\": %.2f, \"humidity\": %.2f }", t, h);
    Serial.print(payload);
    if (!client.publish(mqttSensorTopic, payload)) {
      Serial.print(F("publish failed"));
      errorCounter += 1;
      Serial.print(client.state());
    }
  }
  digitalWrite(LED, LOW);
  if (errorCounter > 5) {
    setup();
  }
}

void loop() {
  if (!client.connected()) {
    connectMqtt();
  }
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    readFromSensor();
  }
}
