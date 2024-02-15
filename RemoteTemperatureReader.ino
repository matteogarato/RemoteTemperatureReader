#include <DHT.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define ssid F("Redacted")
#define password F("Redacted")
#define DHT22_PIN 21  // ESP32 pin GPIO21 connected to DHT22 sensor
#define LED 2
#define mqttPort 1883
#define mqttServer "Redacted"
#define mqttUser "mqtt_user"
#define mqttPassword "Redacted"
#define mqttSensorTopic "home/external/sensor"
//Il sensore funziona, ma ha un formato dati diverso nell'intervallo di temperatura negativo rispetto all'Aosong AM2302.
//Con le solite librerie, ad es. "adafruit" non può visualizzare temperature inferiori allo zero.
//A temperature inferiori allo zero, Databyte 2 e 3, che emettono la temperatura, non sono identici ad Aosong. Byte di dati 2 = F e 3 = FF a -0,1°C. Sospetto che "Byte 2 Lowernibble Bit 3" indichi la temperatura negativa, il resto viene trasmesso in complemento a due.
//Non ho trovato una scheda tecnica o una libreria compatibile.

DHT dht22(DHT22_PIN, DHT22);
WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  pinMode(LED, OUTPUT);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("no wifi"));
    delay(200);
  }
  client.setServer(mqttServer, mqttPort);
  dht22.begin();
}

void loop() {
  digitalWrite(LED, HIGH);
  dht22.read();
  float h = dht22.readHumidity();
  float t = dht22.readTemperature();
  if (isnan(h) || isnan(t) || t > 60) {
    Serial.println(F("Failed to read from DHT sensor!"));
    delay(1000);
    return;
  }
  if (WiFi.status() == WL_CONNECTED) {
    char payload[60];
    snprintf(payload, sizeof(payload), "{\"temperature\": %.2f, \"humidity\": %.2f }", t, h);
    if (!client.connected()) {
      client.connect("ESP32Client", mqttUser, mqttPassword);
    }
    if (!client.publish(mqttSensorTopic, payload)) {
      Serial.print(F("publish failed"));
      Serial.print(client.state());      
    }
  }
  if (client.connected()) {
    client.disconnect();
  }
  digitalWrite(LED, LOW);
  delay(50000);
}
