#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h> // Necesaria para manejar JSON

// --- Configuracion de WiFi y MQTT (Misma que la anterior) ---
const char* ssid = "LunarComms4";
const char* password = "11223344";
const char* mqtt_server = "10.42.0.1";
const int mqtt_port = 1883;
const char* mqtt_topic_publish = "sensor_data"; // Nuevo topic para JSON
const char* client_id = "ESP32_JSON_Publisher";

// --- Variables ---
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;

void setup_wifi() {
  // ... (función setup_wifi idéntica a la anterior) ...
}

void reconnect() {
  // ... (función reconnect idéntica a la anterior) ...
}

// Función que simula la lectura de un sensor
float readTemperature() {
  // Simulación: Valor entre 20.0 y 30.0
  return random(200, 300) / 10.0;
}

float readHumidity() {
  // Simulación: Valor entre 40.0 y 60.0
  return random(400, 600) / 10.0;
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  randomSeed(analogRead(0)); // Inicializa el generador de números aleatorios
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 10000) { // Publica cada 10 segundos
    lastMsg = now;

    // 1. Lee los datos del sensor
    float temp = readTemperature();
    float hum = readHumidity();

    // 2. Crea el documento JSON
    StaticJsonDocument<200> doc; // 200 bytes son suficientes para este caso
    doc["device_id"] = client_id;
    doc["timestamp_ms"] = now;
    doc["temp_C"] = temp;
    doc["humidity_pct"] = hum;

    // 3. Serializa el JSON a una cadena
    char json_buffer[200];
    size_t n = serializeJson(doc, json_buffer);
    
    // 4. Publica el JSON
    Serial.print("Publicando JSON: ");
    Serial.println(json_buffer);
    
    client.publish(mqtt_topic_publish, json_buffer, n, false); // QoS 0
  }
}