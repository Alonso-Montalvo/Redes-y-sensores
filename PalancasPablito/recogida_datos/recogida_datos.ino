#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <Adafruit_LTR390.h>

// =======================================
// PINES I2C FUNCIONALES PARA TU ESP32-S3
// =======================================

// BME280 → Bus principal
#define BME_SDA 11
#define BME_SCL 10

// LTR390 → Bus secundario
#define LTR_SDA 20
#define LTR_SCL 21

TwoWire I2C_LTR(1);

// =======================================
// WIFI / MQTT
// =======================================

const char* ssid = "iPhone de Alonso";
const char* password = "12345678";

const char* mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;

const char* TOPIC_TIME = "TimeNow";
const char* TOPIC_SENSORS = "Enviromental Sensors Network";   // 

WiFiClient espClient;
PubSubClient client(espClient);

Adafruit_BME280 bme;
Adafruit_LTR390 ltr;

String lastUTC = "";

// =======================================
// WIFI
// =======================================

void connectWiFi() {
  Serial.print("Conectando a WiFi ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }

  Serial.println("\n[OK] WiFi conectado");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// =======================================
// MQTT CALLBACK
// =======================================

void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++)
    msg += (char)payload[i];

  StaticJsonDocument<128> doc;

  if (!deserializeJson(doc, msg)) {
    if (doc.containsKey("TimeNow")) {
      lastUTC = doc["TimeNow"].as<String>();
      Serial.print("UTC actualizado: ");
      Serial.println(lastUTC);
    }
  }
}

// =======================================
// RECONEXIÓN MQTT
// =======================================

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.println("Reconectando a MQTT...");

    if (client.connect("ESP32S3_A1")) {
      client.subscribe(TOPIC_TIME);
      Serial.println("Suscrito a TimeNow");
    } else {
      delay(2000);
    }
  }
}

// =======================================
// SETUP
// =======================================

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("🚀 ESP32-S3 A1 – BME280 + LTR390");

  connectWiFi();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  // 1. Iniciar BME280
  Wire.begin(BME_SDA, BME_SCL);

  if (!bme.begin(0x76, &Wire)) {
    if (!bme.begin(0x77, &Wire)) {
      Serial.println("❌ ERROR: BME280 no detectado");
      while (1);
    }
  }

  Serial.println("✔️ BME280 detectado");

  // 2. Iniciar LTR390
  I2C_LTR.begin(LTR_SDA, LTR_SCL);

  if (!ltr.begin(&I2C_LTR)) {
    Serial.println("❌ ERROR: LTR390 no detectado");
    while (1);
  }

  Serial.println("✔️ LTR390 detectado");
  ltr.setMode(LTR390_MODE_UVS);
}

// =======================================
// LOOP
// =======================================

void loop() {
  if (!client.connected()) reconnectMQTT();
  client.loop();

  // --------------------------
  // Lecturas sensores
  // --------------------------

  float temp = bme.readTemperature();
  float hum  = bme.readHumidity();
  float pres = bme.readPressure() / 100.0F;

  if (!ltr.newDataAvailable()) delay(20);
  uint32_t uvs = ltr.readUVS();
  float uvi = uvs / 2300.0;

  if (lastUTC.length() == 0) return;

  // --------------------------
  // CREAR JSON
  // --------------------------

  StaticJsonDocument<256> doc;

  doc["ID"] = "A1";
  doc["Tiempo_UTC"] = lastUTC;
  doc["Temp_C"] = temp;
  doc["Humidity_Per"] = hum;
  //doc["Pressure_hPa"] = pres;
  //doc["UVS_raw"] = uvs;
  doc["UVI"] = uvi;

  char out[256];
  serializeJson(doc, out);

  // --------------------------
  // PUBLICAR MQTT
  // --------------------------

  bool ok = client.publish(TOPIC_SENSORS, out);
  Serial.println(out);
  Serial.println(ok ? "MQTT OK" : "MQTT FAIL");
  Serial.println("----------------------");

  delay(10000);
}
