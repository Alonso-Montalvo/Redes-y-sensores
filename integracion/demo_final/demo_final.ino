#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_LTR390.h>
#include <time.h>
#include <ArduinoJson.h>

// ----------------------------------------------------
// CONFIGURACIÓN DE RED Y MQTT (DEMOSTRACIÓN)
// ----------------------------------------------------
#define WIFI_SSID "LunarLink"
#define WIFI_PASS "11223344"
const char* mqtt_server = "192.168.0.88";
const int mqtt_port = 1883; 

// --- Ajustes del Sensor y Topics ---
#define DEVICE_ID "A2_DEMO" 
#define FACTOR_CALIBRACION 2300.0
#define MQTT_QOS 1 
const char* topic_publish = "Enviromental Sensors Network";
const char* topic_time = "TimeNow";

// --- Variables ---
Adafruit_LTR390 ltr = Adafruit_LTR390();
WiFiClient espClient;
PubSubClient client(espClient);
struct tm timeinfo;
bool timeSynced = false;
unsigned long lastSend = 0;
const unsigned long SEND_INTERVAL = 20000;

// ----------------------------------------------------
// FUNCIONES AUXILIARES
// ----------------------------------------------------

// Función para calcular día del año
int dayOfYear(struct tm *t) {
    int y = t->tm_year + 1900;
    int m = t->tm_mon + 1;
    int d = t->tm_mday;
    static const int daysBeforeMonth[] = {0,31,59,90,120,151,181,212,243,273,304,334};
    int doy = daysBeforeMonth[m-1] + d;
    if((m>2) && (y%4==0 && (y%100 !=0 || y%400==0))) doy++;
    return doy;
}

// Callback MQTT para mensajes entrantes (RECIBE TimeNow)
void callback(char* topic, byte* payload, unsigned int length) {
    char msg[length+1];
    memcpy(msg, payload, length);
    msg[length] = '\0';
    
    if(strcmp(topic, topic_time) == 0) {
        // ASUMIMOS que TimeNow envía el tiempo en formato UNIX TIMESTAMP (entero)
        time_t unix_time = atol(msg);
        
        // Sincronizar el reloj interno del ESP32
        struct timeval tv = { unix_time, 0 };
        settimeofday(&tv, NULL);
        
        // Obtener la hora sincronizada en la estructura tm
        if(getLocalTime(&timeinfo)) {
            timeSynced = true;
            Serial.print("\n[TIME] HORA SINCRONIZADA por MQTT: ");
            Serial.println(asctime(&timeinfo));
        }
    }
}

// Reconexión WiFi (Estándar)
void setup_wifi() {
    Serial.begin(115200);
    delay(10);
    Serial.print("Conectando a ");
    Serial.println(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    while(WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi Conectada!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
}

// Reconexión MQTT (Se suscribe a TimeNow)
void reconnectMQTT() {
    while(!client.connected()) {
        Serial.println("Conectando a MQTT...");
        if(client.connect(DEVICE_ID)) {
            Serial.println("MQTT Conectado.");
            // 1. Suscribirse a TimeNow
            client.subscribe(topic_time); 
            Serial.print("Suscrito a: ");
            Serial.println(topic_time);
        } else {
            Serial.print("Error de conexión, rc=");
            Serial.print(client.state());
            delay(2000);
        }
    }
}

// ====================================================
// SETUP Y LOOP
// ====================================================

void setup() {
    setup_wifi();

    // Configurar MQTT
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);

    // Inicializar sensor
    Wire.begin();
    if(!ltr.begin()) { Serial.println("Error: No se detecta LTR390"); while(1) delay(10);}
    ltr.setMode(LTR390_MODE_UVS);
    ltr.setGain(LTR390_GAIN_18);
    ltr.setResolution(LTR390_RESOLUTION_20BIT);
    Serial.println("Sensor LTR390 inicializado.");

    // Intentar sincronizar hora NTP (por si TimeNow no está activo)
    configTime(0,0,"pool.ntp.org");
    if(getLocalTime(&timeinfo)) {
        timeSynced = true;
        Serial.println("Hora inicial sincronizada por NTP.");
    }
}

void loop() {
    if(!client.connected()) reconnectMQTT();
    client.loop(); // Procesa mensajes entrantes (ej. TimeNow)

    // Enviar datos cada 20s
    unsigned long now_ms = millis();
    if(now_ms - lastSend >= SEND_INTERVAL) {
        lastSend = now_ms;

        // 2. Lectura del Sensor
        uint32_t uvs = ltr.readUVS();
        float uvi = uvs / FACTOR_CALIBRACION;

        // 3. Obtener la hora (sincronizada por MQTT o NTP)
        // Actualiza la estructura timeinfo y obtiene la hora UNIX actual
        time_t current_unix_time = time(NULL); 
        getLocalTime(&timeinfo, current_unix_time);
        
        // 4. Formatear la hora
        char timeUTC[30];
        strftime(timeUTC, sizeof(timeUTC), "%Y-%m-%d %H:%M:%S", &timeinfo);
        
        // 5. Crear el Payload JSON con la hora y datos
        StaticJsonDocument<256> doc;
        doc["ID"] = DEVICE_ID;
        doc["Location"] = "Profesor_Despacho";
        doc["Tiempo_UTC"] = timeUTC;
        doc["Timestamp_Unix"] = current_unix_time;
        doc["UVI"] = round(uvi * 100.0) / 100.0;

        char payload[256];
        size_t n = serializeJson(doc, payload);

        // 6. Publicar al Topic de la Red
        if(client.publish(topic_publish, (const uint8_t*)payload, n, false, MQTT_QOS)) {
            Serial.print("\n[MQTT] PUBLICACIÓN EXITOSA: ");
            Serial.println(payload);
        } else {
            Serial.println("\n[MQTT] ❌ Fallo en la publicación.");
        }
    }
}