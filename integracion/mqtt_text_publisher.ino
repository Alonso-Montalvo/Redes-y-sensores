#include <WiFi.h>
#include <PubSubClient.h>  // Librería para MQTT
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h> // Librería para el sensor
#include <ArduinoJson.h>   // Librería para crear el formato JSON

// ----------------------------------------------------
// CONFIGURACIÓN DE RED Y MQTT
// ----------------------------------------------------
const char* ssid = "LunarComms4";           [cite_start]// SSID de la red [cite: 1]
const char* password = "11223344";       [cite_start]// Contraseña de la red [cite: 2]
const char* mqtt_server = "broker.emqx.io";      // Host del broker MQTT
const char* mqtt_topic = "data/sensor_a1";  // Topic para publicar los datos
const int MQTT_QOS = 1;                     // Calidad de Servicio: 1 (At least once)

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
int MONTH = 11;      [cite_start]// Noviembre [cite: 7]
int DAY = 5;
int HOUR = 16;
int MINUTE = 57;
int SECOND = 00;     [cite_start]// Segundo inicial [cite: 8]
[cite_start]int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; [cite: 9]
unsigned long lastSecondUpdate = 0;

// ====================================================
// FUNCIONES DE TIEMPO (de 'dar_timestamp')
// ====================================================

bool isLeapYear(int year) {
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)[cite_start]; [cite: 12]
}

int getDayOfYear(int year, int month, int day) {
  int doy = 0;
  [cite_start]for (int i = 0; i < month - 1; i++) { [cite: 13]
    [cite_start]doy += daysInMonth[i]; [cite: 14]
  }
  doy += day;
  if (isLeapYear(year) && month > 2) doy++;
  return doy;
}

void incrementTime() {
  [cite_start]SECOND++; [cite: 15]
  if (SECOND >= 60) {
    SECOND = 0;
    [cite_start]MINUTE++; [cite: 15]
  }
  if (MINUTE >= 60) {
    MINUTE = 0;
    [cite_start]HOUR++; [cite: 16]
  }
  if (HOUR >= 24) {
    HOUR = 0;
    DAY++;
    
    int maxDay = daysInMonth[MONTH - 1];
    [cite_start]if (MONTH == 2 && isLeapYear(YEAR)) maxDay = 29; [cite: 18]
    
    if (DAY > maxDay) {
      DAY = 1;
      [cite_start]MONTH++; [cite: 19]
      if (MONTH > 12) {
        MONTH = 1;
        [cite_start]YEAR++; [cite: 20]
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
  WiFi.disconnect(true); [cite_start]// Desconectar para establecer nueva conexión [cite: 3]
  WiFi.begin(ssid, password); [cite_start]// Conexión WPA/WPA2 simple con contraseña [cite: 4]

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
  [cite_start]Serial.println(F("\nWiFi Conectada!")); [cite: 5]
  Serial.print(F("IP Address: "));
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");
    // El ID del cliente MQTT es el ID del dispositivo
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

  // Inicializar BME280 (de 'programa_estandarizado')
  [cite_start]WireBME.begin(BME_SDA, BME_SCL); [cite: 30]
  [cite_start]if (!bme.begin(0x76, &WireBME)) { [cite: 30]
    Serial.println("❌ No se encontró BME280 en 0x76. Probando 0x77...");
    [cite_start]if (!bme.begin(0x77, &WireBME)) { [cite: 31]
      Serial.println("❌ No se encontró ningún BME280. Deteniendo.");
      [cite_start]while (1) delay(10); [cite: 32]
    }
  }
  [cite_start]Serial.println("✅ BME280 y MQTT inicializados."); [cite: 33]
  
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
  [cite_start]// Actualizar cada SEGUNDO (para no perder la pista) [cite: 21]
  if (now - lastSecondUpdate >= 1000) { 
    lastSecondUpdate = now;
    [cite_start]incrementTime(); [cite: 22]
  }
  
  // 2. Leer sensor y publicar cada 5 segundos
  [cite_start]// Leer cada 5 segundos [cite: 34]
  if (now - lastMillis >= MEASURE_INTERVAL_MS) {
    lastMillis = now;

    // A. Lectura del sensor
    float temp = bme.readTemperature(); [cite_start]// Leer temperatura [cite: 35]
    float hum  = bme.readHumidity();    [cite_start]// Leer humedad [cite: 36]

    // B. Generación del Timestamp
    int dayOfYear = getDayOfYear(YEAR, MONTH, DAY); [cite_start]// Calcular día del año [cite: 23]
    char timestamp[20];
    sprintf(timestamp, "%04d-%03d-%02d:%02d:%02d", 
            YEAR, dayOfYear, HOUR, MINUTE, SECOND); [cite_start]// Formatear timestamp [cite: 24]

    // C. Creación del JSON Payload
    // Estructura solicitada: { "ID ": A1, "Tiempo_UTC": "...", "Temperatura_C": ..., "Humedad_%": ... }
    StaticJsonDocument<200> doc; 
    
    doc["ID"] = DEVICE_ID;
    doc["Tiempo_UTC"] = timestamp;
    // Redondeo a 1 decimal para el JSON
    doc["Temperatura_C"] = round(temp * 10.0) / 10.0; 
    doc["Humedad_%"] = round(hum * 10.0) / 10.0;     

    // D. Serializar y Publicar
    char jsonBuffer[200];
    serializeJson(doc, jsonBuffer); [cite_start]// Imprime: "Temperatura_C": 22.5, "Humedad_%": 45.2 [cite: 36]

    Serial.print("Publicando JSON: ");
    Serial.println(jsonBuffer);
    
    // CORRECCIÓN: Usar la sobrecarga de publish con longitud del payload y casting
    if (client.publish(mqtt_topic, 
                       (const uint8_t*)jsonBuffer, // Casting del buffer JSON
                       strlen(jsonBuffer),        // Longitud del buffer JSON
                       false,                      // Retained = false
                       MQTT_QOS))                  // QoS = 1
    {
      Serial.println("✅ Publicación exitosa.");
    } else {
      Serial.println("❌ Fallo en la publicación.");
    }
  }
}