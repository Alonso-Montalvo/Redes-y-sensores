#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h> // Necesaria para manejar JSON

// --- Configuracion de WiFi y MQTT (¡ACTUALIZADA!) ---
const char* ssid = "LunarLink"; // Nuevo SSID
const char* password = "11223344"; // Nueva Contraseña
const char* mqtt_server = "192.168.0.88"; // Nuevo Host MQTT (o "LunarComms4" si lo resuelves por DNS)
const int mqtt_port = 1883;
const char* mqtt_topic_publish = "sensor_data";
const char* client_id = "ESP32_JSON_Publisher";

// --- Variables ---
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);

  // Intentar la conexión con el nuevo SSID y Pass
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectada!");
  Serial.print("Direccion IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Bucle hasta que estemos reconectados
  while (!client.connected()) {
    Serial.print("Intentando conexion MQTT...");
    // Intenta conectar con el client_id
    if (client.connect(client_id)) {
      Serial.println("conectado.");
    } else {
      Serial.print("fallo, rc=");
      Serial.print(client.state());
      Serial.println(" Intentando de nuevo en 5 segundos");
      // Espera 5 segundos antes de reintentar
      delay(5000);
    }
  }
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
  // Configura el servidor MQTT con la nueva IP
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
    StaticJsonDocument<200> doc;
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
    
    // CORRECCIÓN del error anterior: se usa casting para convertir char* a const uint8_t*
    client.publish(mqtt_topic_publish, (const uint8_t*)json_buffer, n, false); // QoS 0
  }
}