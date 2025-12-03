#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_LTR390.h>
#include <ArduinoJson.h>
#include <time.h>

#define WIFI_SSID "LunarLink"
#define WIFI_PASS "11223344"
#define DEVICE_ID "A2"
#define FACTOR_CALIBRACION 2300.0
#define RGB_BRIGHTNESS 50

// Sensor UV
Adafruit_LTR390 ltr;

// MQTT
WiFiClient espClient;
PubSubClient client(espClient);
const char* mqtt_server = "192.168.0.88";
const char* topic_sensor = "Enviromental Sensors Network";
const char* topic_time = "TimeNow"; 

// LED RGB
void setRGB(uint8_t r, uint8_t g, uint8_t b) {
  rgbLedWrite(RGB_BUILTIN, r, g, b);
}

void blinkRGB(uint8_t r, uint8_t g, uint8_t b, int msDelay) {
  setRGB(r, g, b);
  delay(msDelay);
  setRGB(0,0,0);
  delay(msDelay);
}

// Tiempo
struct tm timeinfo;
bool timeSynced = false;

// ------------------------
// Convertir DOY -> mes/día
// ------------------------
void convertDOYtoDate(int year, int doy, int &month, int &day) {
  static const int monthDays[] =
    {31,28,31,30,31,30,31,31,30,31,30,31};

  int isLeap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));

  month = 0;
  while (true) {
    int days = monthDays[month];
    if (month == 1 && isLeap) days = 29;

    if (doy > days) {
      doy -= days;
      month++;
    } else break;
  }
  day = doy;
}

// ------------------------
// Callback MQTT
// ------------------------
void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  Serial.printf("Mensaje recibido en %s: %s\n", topic, payload);

  if (String(topic) == "TimeNow") {

    StaticJsonDocument<100> doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
      Serial.println("ERROR: No se pudo parsear JSON TimeNow");
      return;
    }

    const char* rawTime = doc["TimeNow"]; // Ejemplo: "2025-323-15:07:06"
    int year, doy, hh, mm, ss;

    if (sscanf(rawTime, "%d-%d-%d:%d:%d", &year, &doy, &hh, &mm, &ss) != 5) {
      Serial.println("ERROR: Formato TimeNow no válido");
      return;
    }

    // Convertir DOY
    int month, day;
    convertDOYtoDate(year, doy, month, day);

    // Guardar hora
    timeinfo.tm_year = year - 1900;
    timeinfo.tm_mon  = month;
    timeinfo.tm_mday = day;
    timeinfo.tm_hour = hh;
    timeinfo.tm_min  = mm;
    timeinfo.tm_sec  = ss;

    // <<<<<<<< CORRECCIÓN IMPORTANTE >>>>>>>>
    mktime(&timeinfo);   // Calcula tm_yday y normaliza la fecha

    timeSynced = true;

    Serial.printf("Hora sincronizada: %04d-%02d-%02d %02d:%02d:%02d\n",
      year, month+1, day, hh, mm, ss);
  }
}

// ------------------------
// Reconexión MQTT
// ------------------------
void reconnectMQTT() {
  while(!client.connected()) {
    Serial.println("Conectando a MQTT...");
    if(client.connect(DEVICE_ID)) {
      Serial.println("MQTT Conectado");
      client.subscribe(topic_time);
      setRGB(0, RGB_BRIGHTNESS, 0);
    } else {
      Serial.printf("Error conexión MQTT, rc=%d\n", client.state());
      blinkRGB(RGB_BRIGHTNESS, 0, 0, 300);
    }
  }
}

// ------------------------
// SETUP
// ------------------------
void setup() {
  Serial.begin(115200);
  delay(500);

  setRGB(RGB_BRIGHTNESS, 0, 0);

  // Conexión WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while(WiFi.status() != WL_CONNECTED) {
    blinkRGB(RGB_BRIGHTNESS, 0, 0, 300);
  }
  Serial.println("WiFi Conectado");
  setRGB(0, RGB_BRIGHTNESS, 0);

  // MQTT
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // Sensor
  Wire.begin();
  if(!ltr.begin()) {
    Serial.println("ERROR: No se detecta LTR390");
    while(1) delay(10);
  }
  ltr.setMode(LTR390_MODE_UVS);
  ltr.setGain(LTR390_GAIN_18);
  ltr.setResolution(LTR390_RESOLUTION_20BIT);

  // Si no llega TimeNow, usar NTP
  configTime(0, 0, "pool.ntp.org");
  if(getLocalTime(&timeinfo)) timeSynced = true;
}

unsigned long lastSend = 0;
const unsigned long SEND_INTERVAL = 20000;

// ------------------------
// LOOP PRINCIPAL
// ------------------------
void loop() {
  if(!client.connected()) reconnectMQTT();
  client.loop();

  unsigned long now = millis();
  if(now - lastSend >= SEND_INTERVAL) {
    lastSend = now;

    // Medir UVI
    uint32_t uvs = ltr.readUVS();
    float uvi = uvs / FACTOR_CALIBRACION;

    if(!timeSynced) getLocalTime(&timeinfo);

    // Actualizar la estructura por seguridad
    mktime(&timeinfo);

    // Calcular DOY correcto
    int doy = timeinfo.tm_yday + 1;

    // Formato final
    char timeUTC[30];
    snprintf(timeUTC, sizeof(timeUTC),
      "%04d-%03d-%02d:%02d:%02d",
      timeinfo.tm_year + 1900, doy,
      timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec
    );

    // JSON final
    char payload[150];
    snprintf(payload, sizeof(payload),
      "{ \"ID\":\"%s\", \"Tiempo_UTC\":\"%s\", \"UVI\":%.2f }",
      DEVICE_ID, timeUTC, uvi
    );

    // Enviar
    client.publish(topic_sensor, payload);
    Serial.println(payload);
  }
}

