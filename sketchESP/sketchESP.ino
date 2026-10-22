/*
 * Device ESP32: sensor illuminace, actuator LED, communication MQTT
*/

#include <WiFi.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

const char* mqttClientID = "device_01";
const char* mqttTopicPubSensor = "wot/device_01/properties/illuminance";
const char* mqttTopicPubLed = "wot/device_01/properties/ledState";
const char* mqttTopicSubLed = "wot/device_01/actions/setLed";
const char* mqttTopicPubEventLow = "wot/device_01/events/lowIlluminance";
const char* mqttTopicPubEventHigh = "wot/device_01/events/highIlluminance";

const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

const char* mqttServer = "YOUR_MQTT_BROKER";
const char* mqttUser = "YOUR_MQTT_USER";
const char* mqttPassword = "YOUR_MQTT_PASSWORD";
const int mqttPort = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

constexpr uint8_t LED_PIN = 2;
constexpr unsigned long SENSOR_INTERVAL = 10000UL;

constexpr int LOW_ILLUMINANCE_LIMIT = 100;
constexpr int HIGH_ILLUMINANCE_LIMIT = 800;

unsigned long lastPubMQTT = 0;

int illuminanceLux = 0;
bool hasIlluminanceReading = false;

enum class LightZone : uint8_t {
  UNKNOWN,
  LOW_ILLUMINANCE,
  NORMAL_ILLUMINANCE,
  HIGH_ILLUMINANCE
};

LightZone currentLightZone = LightZone::UNKNOWN;

/* Prototypes */
void connectWiFi();
void connectBroker();
void callback(char* topic, byte* payload, unsigned int length);
void checkConnections();
void checkSensors();
LightZone classifyIlluminance(int value);
void processIlluminanceState(int value);
void publishIlluminanceEvent(const char* topic, int value);
void pubMQTT();
 
void connectWiFi(){
  if (WiFi.status() == WL_CONNECTED){
    return;
  } else {
    Serial.print("Conectando a Rede WiFi");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED){
      delay(100);
      Serial.print(".");
    }
    Serial.println();
    Serial.println("- WIFI CONECTADO");
    Serial.print("Rede: ");
    Serial.println(ssid);
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  }
}

void connectBroker(){  
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  if (client.connected()) {
    return;
  } else {
    while (!client.connected()){  
      Serial.println("Connecting to MQTT Broker...");    
      if (client.connect(mqttClientID, mqttUser, mqttPassword )){      
        Serial.println("- BROKER CONNECTED");
        client.subscribe(mqttTopicSubLed, 1); //nivel de qualidade: QoS 1
      } else {
        Serial.print("Connection failure - State: ");      
        Serial.println(client.state());      
        delay(2000);    
      }
    }
  }
}

void checkConnections(){
  connectWiFi();
  if (!client.connected()) {
    connectBroker();
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  if (strcmp(topic, mqttTopicSubLed) != 0) {
    Serial.println("[MQTT] Topic not recognized");
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload, length);

  if (error) {
    Serial.print("[JSON] Invalid payload: ");
    Serial.println(error.c_str());
    return;
  }

    if (!doc.is<const char*>()) {
    Serial.println("[Action] Invalid type");
    return;
  }

  const char* command = doc.as<const char*>();

  if (command == nullptr) {
    Serial.println("[Action] Invalid command");
    return;
  }

  bool requestedState;

  if (strcmp(command, "on") == 0) {
    requestedState = true;
  } else if (strcmp(command, "off") == 0) {
    requestedState = false;
  } else {
    Serial.print("[Action] Value not permitted: ");
    Serial.println(command);
    return;
  }

  digitalWrite(LED_PIN, requestedState ? HIGH : LOW);

  const bool ledReading = digitalRead(LED_PIN);

  Serial.print("[Actuator] LED: ");
  Serial.println(ledReading ? "on" : "off");

  const char* statusPayload = ledReading ? "\"on\"" : "\"off\"";

  if (client.publish(mqttTopicPubLed, statusPayload, true)) {
    Serial.print("[MQTT] Tópico: ");
    Serial.println(mqttTopicPubLed);
    Serial.print("[MQTT] LED: ");
    Serial.println(statusPayload);

  } else {
    Serial.println("[MQTT] Failed to publish LED state");
  }
}

LightZone classifyIlluminance(int value) {
    if (value < LOW_ILLUMINANCE_LIMIT) {
      return LightZone::LOW_ILLUMINANCE;
    }

    if (value > HIGH_ILLUMINANCE_LIMIT) {
      return LightZone::HIGH_ILLUMINANCE;
    }

    return LightZone::NORMAL_ILLUMINANCE;
}

void checkSensors(){
  illuminanceLux = random(0, 1001);

  hasIlluminanceReading = true;

  Serial.print("[Sensor] Iluminance: ");
  Serial.print(illuminanceLux);
  Serial.println(" lx");

  pubMQTT();

  processIlluminanceState(illuminanceLux);
}

void processIlluminanceState(int value) {
  const LightZone newZone = classifyIlluminance(value);
  if (newZone == currentLightZone) {
    return;
  }

  currentLightZone = newZone;

  switch (newZone) {
    case LightZone::LOW_ILLUMINANCE:
      Serial.println("[State] LOW_ILLUMINANCE");
      publishIlluminanceEvent(mqttTopicPubEventLow, value);
      break;
    case LightZone::NORMAL_ILLUMINANCE:
      Serial.println("[State] NORMAL_ILLUMINANCE");
      // Não há evento de retorno definido na TD.
      break;
    case LightZone::HIGH_ILLUMINANCE:
      Serial.println("[State] HIGH_ILLUMINANCE");
      publishIlluminanceEvent(mqttTopicPubEventHigh, value);
      break;
    case LightZone::UNKNOWN:
      break;
  }
}

void publishIlluminanceEvent(const char* topic, int value) {
  char payload[16];

  snprintf(payload, sizeof(payload), "%d", value);

  if (client.publish(topic, payload, false)) {
    Serial.print("[Event] Topic: ");
    Serial.println(topic);
    Serial.print("[Event] Payload: ");
    Serial.println(payload);
  } else {
    Serial.println("[Event] Failed to publish the notification");
  }
}

void pubMQTT() {
  if (!hasIlluminanceReading) {
    return;
  }

  char payload[16];

  snprintf(payload, sizeof(payload), "%d", illuminanceLux);

  if (client.publish(mqttTopicPubSensor, payload, true)) {
    Serial.print("[MQTT] Tópico: ");
    Serial.println(mqttTopicPubSensor);

    Serial.print("[MQTT] Iluminância: ");
    Serial.print(payload);
    Serial.println(" lx");
  } else {
    Serial.println("[MQTT] Falha ao publicar a iluminância.");
  }
}

void setup(){
  Serial.begin(115200);  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN,LOW);

  randomSeed(esp_random());

  connectWiFi();
  connectBroker();

  const char* initialLedPayload = "\"off\"";
  client.publish(mqttTopicPubLed, initialLedPayload, true);

  Serial.println("[System] Initialization complete");
}

void loop(){
  client.loop();

  checkConnections();

  if ((millis() - lastPubMQTT) > SENSOR_INTERVAL) {
    lastPubMQTT = millis();
    checkSensors();
  }
}
