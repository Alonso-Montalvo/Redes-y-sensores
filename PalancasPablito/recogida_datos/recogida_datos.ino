#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <Adafruit_LTR390.h>

#include <Adafruit_NeoPixel.h>

// ===============================
// LED RGB INTEGRADO (WS2812)
// ===============================
#define LED_PIN 48
#define LED_NUM 1
Adafruit_NeoPixel led(LED_NUM, LED_PIN, NEO_GRB + NEO_KHZ800);

void setColor(uint8_t r, uint8_t g, uint8_t b) {
  led.setPixelColor(0, led.Color(r, g, b));
  led.show();
}

void blinkColor(uint8_t r, uint8_t g, uint8_t b, int ms) {
  setColor(r, g, b);
  delay(ms);
  setColor(0, 0, 0);
  delay(ms);
}

// ===============================
// PINES I2C
// ===============================
// Para BME280
#define BME_SDA 11
#define BME_SCL 10

// Para LTR390
#define LTR_SDA 20
#define LTR_SCL 21
#define FACTOR_CALIBRACION 2300.0

TwoWire I2C_LTR(1);

// ===============================
// WIFI / MQTT
// ===============================
const char* ssid = "iPhone de Alonso";
const char* password = "12345678";

const char* mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;

const char* TOPIC_TIME = "TimeNow";
const char* TOPIC_SENSORS = "Enviromental Sensors Network";

WiFiClient espClient;
PubSubClient client(espClient);

// Objetos para los sensores
Adafruit_BME280 bme;
Adafruit_LTR390 ltr;
String lastUTC = ""; 

// ===============================
// WIFI
// ===============================
void connectWiFi() {
  Serial.print("Conectando a WiFi ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    blinkColor(0, 0, 255, 200);  // 🔵 Azul parpadeando
  }

  Serial.println("\n[OK] WiFi conectado");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  setColor(0, 255, 0);  // 🟢 Verde fijo
}

// ===============================
// CALLBACK MQTT
// ===============================
void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) msg += (char)payload[i];

  // Aumentamos también aquí el buffer por si llega un mensaje largo de tiempo
  StaticJsonDocument<256> doc;
  
  DeserializationError error = deserializeJson(doc, msg);
  
  if (!error) { 
    if (doc.containsKey("TimeNow")) {
      lastUTC = doc["TimeNow"].as<String>();
      Serial.print("UTC recibido: ");
      Serial.println(lastUTC);
    }
  } else {
    Serial.print("Error JSON: ");
    Serial.println(error.c_str());
  }
}

// ===============================
// RECONNECT MQTT
// ===============================
void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");
    blinkColor(255, 128, 0, 200);   // 🟠 Naranja parpadeando

    if (client.connect("ESP32S3_A1")) {
      Serial.println("conectado");
      client.subscribe(TOPIC_TIME);
      Serial.println("Suscrito a: TimeNow");
      setColor(0, 255, 0); 
    } else {
      Serial.print("falló, rc=");
      Serial.print(client.state());
      Serial.println(" reintentando en 2s");
      delay(2000);
    }
  }
}

// ===============================
// SETUP
// ===============================
void setup() {
  Serial.begin(115200);
  delay(500);

  led.begin();
  led.show();
  setColor(0, 0, 0);

  Serial.println("🚀 ESP32-S3 A1 – BME280 + LTR390");

  connectWiFi();

  // --- CONFIGURACIÓN MQTT CRÍTICA ---
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  
  // ¡AUMENTAMOS EL TAMAÑO DEL BUFFER MQTT! 
  // Por defecto es 256 bytes, lo subimos a 512 para que quepa el JSON completo con la presión.
  client.setBufferSize(512); 

  // Inicialización de BME280
  Wire.begin(BME_SDA, BME_SCL); 
  if (!bme.begin(0x76, &Wire)) {
    if (!bme.begin(0x77, &Wire)) {
      Serial.println("❌ BME280 error");
      while (1) blinkColor(255, 0, 0, 100);
    }
  }
  Serial.println("✔️ BME280 OK");

  // Inicialización de LTR390
  I2C_LTR.begin(LTR_SDA, LTR_SCL); 
  if (!ltr.begin(&I2C_LTR)) {
    Serial.println("❌ LTR390 error");
    while (1) blinkColor(255, 0, 0, 100);
  }
  Serial.println("✔️ LTR390 OK");
  
  ltr.setMode(LTR390_MODE_UVS); 
}

// ===============================
// LOOP
// ===============================
void loop() {
  if (!client.connected()) reconnectMQTT();
  client.loop();

  // Lecturas
  float temp = bme.readTemperature();
  float hum = bme.readHumidity();
  float pres = bme.readPressure() / 100.0F; // hPa

  if (!ltr.newDataAvailable()) {
    delay(20);
    return;
  }
  uint32_t uvs = ltr.readUVS(); 
  float uvi = uvs / FACTOR_CALIBRACION;     

  if (lastUTC.length() == 0) {
    Serial.println("Esperando tiempo UTC...");
    setColor(0, 0, 255); 
    delay(1000);
    return;
  }

  // Lógica LED (Alertas)
  bool peligro = false;
  bool aviso = false;

  if (temp < 0 || temp > 35) peligro = true;
  else if (temp < 10 || temp > 30) aviso = true;

  if (hum < 20 || hum > 90) peligro = true;
  else if (hum < 30 || hum > 80) aviso = true;

  if (uvi > 8.0) peligro = true;
  else if (uvi > 5.0) aviso = true;

  if (peligro) blinkColor(255, 0, 0, 300);
  else if (aviso) setColor(255, 128, 0);
  else setColor(0, 255, 0);

  // ===============================
  // PUBLICAR DATOS MQTT (CORREGIDO)
  // ===============================

  // 1. Aumentamos el tamaño del documento JSON
  StaticJsonDocument<512> doc;

  doc["ID"] = "A1";
  doc["Tiempo_UTC"] = lastUTC;
  
  // Redondeamos a 2 decimales para ahorrar espacio y limpiar datos
  doc["Temp_C"] = ((int)(temp * 100 + 0.5)) / 100.0;
  doc["Humidity_Per"] = ((int)(hum * 100 + 0.5)) / 100.0;
  doc["UVI"] = ((int)(uvi * 100 + 0.5)) / 100.0;
  
  // AHORA SÍ INCLUIMOS LA PRESIÓN
  doc["Pressure_hPa"] = ((int)(pres * 100 + 0.5)) / 100.0;

  // 2. Aumentamos el tamaño del buffer de salida de caracteres
  char out[512];
  serializeJson(doc, out);

  // Publicar
  bool ok = client.publish(TOPIC_SENSORS, out);
  
  Serial.print("JSON Size: ");
  Serial.print(strlen(out)); // Útil para depurar el tamaño real
  Serial.println(" bytes");
  Serial.println(out);
  Serial.println(ok ? "MQTT OK" : "MQTT FAIL (Buffer overflow?)");
  Serial.println("----------------------");

  delay(2000); 
}