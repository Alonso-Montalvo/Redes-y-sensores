#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// Pines de conexión I²C
#define I2C_SDA 21
#define I2C_SCL 20

// LED grande del ESP32-S3 DevKitC-1
#define LED_VERDE 48

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme;

// Intervalos de tiempo
const float intervalo_lectura = 0.1;   // segundos
const float intervalo_media = 5.0;     // segundos
const int num_muestras = intervalo_media / intervalo_lectura; // 50 lecturas

// Variables acumuladoras
float suma_temp = 0;
float suma_hum = 0;
float suma_pres = 0;
int contador = 0;

// Temporizadores
unsigned long t_ultimo = 0;
unsigned long t_led = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  pinMode(LED_VERDE, OUTPUT);
  digitalWrite(LED_VERDE, LOW);

  // Inicializar bus I²C
  Wire.begin(I2C_SDA, I2C_SCL);

  Serial.println("Inicializando BME280...");

  // Dirección fija 0x76 (cámbiala a 0x77 si tu escáner lo indicó)
  if (!bme.begin(0x76)) {
    Serial.println("❌ No se encontró el BME280 en 0x76. Verifica el cableado o la dirección.");
    while (1) delay(10);
  }

  Serial.println("✅ BME280 inicializado correctamente.");
  Serial.println("Midiendo cada 0.1 s y mostrando media cada 5 s...\n");
}

void loop() {
  unsigned long ahora = millis();

  // Parpadeo LED cada 100 ms
  if (ahora - t_led >= 100) {
    digitalWrite(LED_VERDE, !digitalRead(LED_VERDE));
    t_led = ahora;
  }

  // Lectura del sensor cada 100 ms
  if (ahora - t_ultimo >= 100) {
    t_ultimo = ahora;

    float t = bme.readTemperature();
    float h = bme.readHumidity();
    float p = bme.readPressure() / 100.0F;

    suma_temp += t;
    suma_hum += h;
    suma_pres += p;
    contador++;

    // Cada 5 s (50 lecturas)
    if (contador >= num_muestras) {
      float media_t = suma_temp / contador;
      float media_h = suma_hum / contador;
      float media_p = suma_pres / contador;

      Serial.print("Media (últimos 5 s) → ");
      Serial.print("Temp: "); Serial.print(media_t, 2); Serial.print(" °C, ");
      Serial.print("Hum: "); Serial.print(media_h, 2); Serial.print(" %, ");
      Serial.print("Pres: "); Serial.print(media_p, 2); Serial.println(" hPa");

      suma_temp = suma_hum = suma_pres = 0;
      contador = 0;
    }
  }
}
