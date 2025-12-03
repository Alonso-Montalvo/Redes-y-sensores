#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "time.h" // Necesaria para obtener la hora del servidor NTP

// --- Configuracion de WiFi y MQTT ---
const char* ssid = "LunarComms4";
const char* password = "11223344";
const char* mqtt_server = "broker.emqx.io";
const char* mqtt_topic_publish = "Enviromental Sensors Network"; // Topic final
const char* client_id = "Sensor_Astro_ID_007"; // ID único para su sensor
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600; // GMT + 1 (Horario de Invierno, ajusta si es Verano)
const int   daylightOffset_sec = 3600;

// --- Variables ---
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;

// (Funciones setup_wifi y reconnect idénticas a las anteriores)

void init_time() {
  // Conecta a NTP para obtener la hora precisa
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    Serial.println("Esperando hora NTP...");
    delay(1000);
  }
  Serial.println("Hora NTP sincronizada.");
}

float readOxygenLevel() {
  return random(190, 210) / 10.0; // Simulación de % de oxígeno
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  init_time(); // Sincroniza la hora
  client.setServer(mqtt_server, 1883);
  randomSeed(analogRead(0));
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 20000) { // Publica cada 20 segundos
    lastMsg = now;

    // 1. Obtener la marca de tiempo (Timestamp UNIX)
    time_t now_unix = time(NULL);
    
    // 2. Leer los datos del sensor
    float oxygen = readOxygenLevel();
    
    // 3. Crea el documento JSON
    StaticJsonDocument<256> doc;
    doc["sensor_id"] = client_id;
    doc["timestamp_unix"] = now_unix;
    doc["location"] = "Habitacion_Principal";
    
    // Datos del sensor (Tu sensor específico)
    doc["data"]["oxigen_percent"] = oxygen; 
    doc["data"]["status"] = "OK"; 

    // 4. Serializa el JSON
    char json_buffer[256];
    size_t n = serializeJson(doc, json_buffer);
    
    // 5. Publica el JSON
    Serial.print("Publicando JSON a Environmental Sensors Network: ");
    Serial.println(json_buffer);
    
    client.publish(mqtt_topic_publish, json_buffer, n, false); 
  }
}