#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ArduinoJson.h> // Necesaria para el JSON

// ----------------------------------------------------
// CONFIGURACIÓN DE RED Y MQTT
// ----------------------------------------------------
const char* ssid = "LunarComms4";
const char* password = "11223344";
const char* mqtt_server = "broker.emqx.io";
const char* mqtt_topic = "data/sensor_a1"; // Nuevo topic para los datos
const int MQTT_QOS = 1;

WiFiClient espClient;
PubSubClient client(espClient);

// Intervalo de muestreo y publicación (5 segundos)
const unsigned long MEASURE_INTERVAL_MS = 5000;
unsigned long lastMillis = 0;

// ----------------------------------------------------
// CONFIGURACIÓN BME280 (de 'programa_estandarizado')
// ----------------------------------------------------
#define BME_SDA 45
#define BME_SCL 0
Adafruit_BME280 bme;
TwoWire WireBME = TwoWire(1);
const char* DEVICE_ID = "A1";

// ----------------------------------------------------
// CONFIGURACIÓN TIMESTAMP (de 'dar_timestamp')
// ----------------------------------------------------
int YEAR = 2025;
int MONTH = 11;
int DAY = 5;
int HOUR = 16;
int MINUTE = 57;
int SECOND = 00;
int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
unsigned long lastSecondUpdate = 0;

// ====================================================
// FUNCIONES DE TIEMPO (de 'dar_timestamp')
// ====================================================

bool isLeapYear(int year) {
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int getDayOfYear(int year, int month, int day) {
  int doy = 0;
  for (int i = 0; i < month - 1; i++) {
    doy += daysInMonth[i];
  }
  doy += day;
  if (isLeapYear(year) && month > 2) doy++;
  return doy;
}

void incrementTime() {
  SECOND++;
  if (SECOND >= 60) {
    SECOND = 0;
    MINUTE++;
  }
  if (MINUTE >= 60) {
    MINUTE = 0;
    HOUR++;
  }
  if (HOUR >= 24) {
    HOUR = 0;
    DAY++;
    
    int maxDay = daysInMonth[MONTH - 1];
    if (MONTH == 2 && isLeapYear(YEAR)) maxDay = 29;
    
    if (DAY > maxDay) {
      DAY = 1;
      MONTH++;
      if (MONTH > 12) {
        MONTH = 1;
        YEAR++;
      }
    }
  }
}

// ====================================================
// FUNCIONES DE CONEXIÓN Y SETUP
// ====================================================

void setup_wifi() {
  Serial.begin(115200);
  delay(10);
  Serial.print(F("Conectando a: "));
  Serial.println(ssid);
  WiFi.disconnect(true);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
  Serial.println(F("\nWiFi Conectada."));
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");
    if (client.connect(DEVICE_ID)) {
      Serial.println("conectado!");
    } else {
      Serial.print("falló, rc=");
      Serial.print(client.state());
      Serial.println(" Intentando de nuevo en 5s");
      delay(5000);
    }
  }
}

void setup() {
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);

  // Inicializar BME280
  WireBME.begin(BME_SDA, BME_SCL);
  if (!bme.begin(0x76, &WireBME) && !bme.begin(0x77, &WireBME)) {
    Serial.println("❌ No se encontró BME280. Deteniendo.");
    while (1) delay(10);
  }
  Serial.println("✅ BME280 y MQTT inicializados.");
  
  lastSecondUpdate = millis();
}

// ====================================================
// BUCLE PRINCIPAL
// ====================================================

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop(); // Mantiene la conexión MQTT
  
  unsigned long now = millis();
  
  // 1. Actualizar el tiempo cada segundo
  if (now - lastSecondUpdate >= 1000) {
    lastSecondUpdate = now;
    incrementTime();
  }
  
  // 2. Leer sensor y publicar cada 5 segundos
  if (now - lastMillis >= MEASURE_INTERVAL_MS) {
    lastMillis = now;

    // A. Lectura del sensor
    float temp = bme.readTemperature();
    float hum  = bme.readHumidity();

    // B. Generación del Timestamp
    int dayOfYear = getDayOfYear(YEAR, MONTH, DAY);
    char timestamp[20];
    sprintf(timestamp, "%04d-%03d-%02d:%02d:%02d", 
            YEAR, dayOfYear, HOUR, MINUTE, SECOND);

    // C. Creación del JSON Payload
    // Tamaño suficiente para el JSON solicitado
    StaticJsonDocument<200> doc; 
    
    // Estructura JSON solicitada: { "ID ": A1, "Tiempo_UTC": "...", "Temperatura_C": ..., "Humedad_%": ... }
    doc["ID"] = DEVICE_ID;
    doc["Tiempo_UTC"] = timestamp;
    doc["Temperatura_C"] = round(temp * 10.0) / 10.0; // Redondea a 1 decimal
    doc["Humedad_%"] = round(hum * 10.0) / 10.0;     // Redondea a 1 decimal

    // D. Serializar y Publicar
    char jsonBuffer[200];
    serializeJson(doc, jsonBuffer);

    Serial.print("Publicando JSON: ");
    Serial.println(jsonBuffer);
    
    if (client.publish(mqtt_topic, jsonBuffer, false, MQTT_QOS)) {
      Serial.println("✅ Publicación exitosa.");
    } else {
      Serial.println("❌ Fallo en la publicación.");
    }
  }
}