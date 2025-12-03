#include <WiFi.h>
#include <PubSubClient.h>

// --- Configuracion de WiFi ---
const char* ssid = "LunarComms4";
const char* password = "11223344";

// --- Configuracion de MQTT ---
const char* mqtt_server = "10.42.0.1";
const int mqtt_port = 1883;
const char* mqtt_topic_publish = "text_test";
const char* client_id = "ESP32_Text_Publisher";

// --- Variables ---
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
int value = 0;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);

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
    // Intenta conectar
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

void setup() {
  Serial.begin(115200);
  setup_wifi();
  // Configura el servidor y puerto del broker
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop(); // Maneja el tráfico de red y keep-alive

  long now = millis();
  if (now - lastMsg > 5000) { // Publica cada 5 segundos
    lastMsg = now;
    value++;
    
    // Construye el mensaje
    String message = "Hola desde ESP32 - Mensaje #" + String(value);
    
    Serial.print("Publicando mensaje: ");
    Serial.println(message);
    
    // Publica al topic con QoS 1 (QoS 0 o 2 son otras opciones)
    client.publish(mqtt_topic_publish, message.c_str(), 1); 
  }
}