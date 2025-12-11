import paho.mqtt.client as mqtt
import json
import time
import random

# --- CONFIGURACIÓN ---
BROKER = "broker.emqx.io"
TOPIC = "Enviromental Sensors Network"
SENSOR_ID = "B2"  # ID diferente al A1 para diferenciarlo
UBICACION = "Laboratorio Bio-Regenerativo"

def connect_mqtt():
    """Conecta al broker MQTT."""
    try:
        # Usamos la API nueva para evitar warnings, igual que en tu app
        client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
        client.connect(BROKER, 1883, 60)
        return client
    except Exception as e:
        print(f"Error conectando al broker: {e}")
        return None

def generar_datos_simulados():
    """Genera valores realistas de calidad del aire."""
    # CO2: Nivel base ~400ppm (aire libre), ~600-800ppm (interior habitado)
    co2 = round(random.uniform(400, 850), 2)
    
    # CO: Monóxido de carbono. Ideal 0. Peligroso > 50.
    co = round(random.uniform(0, 5), 3)
    
    # Añadimos un pico ocasional para simular eventos
    if random.random() > 0.95:
        co2 += 200 # Pico de gente entrando
        
    return co, co2

def run_simulator():
    client = connect_mqtt()
    if not client:
        return

    print(f"--- Iniciando Simulación de Sensor {SENSOR_ID} ---")
    print(f"Destino: {BROKER} -> {TOPIC}")
    print("Enviando datos de CO y CO2...")

    try:
        while True:
            # 1. Generar datos
            co_val, co2_val = generar_datos_simulados()
            
            # 2. Crear estructura JSON
            payload = {
                "ID": SENSOR_ID,
                "Location": UBICACION,
                "Tiempo_UTC": time.strftime("%Y-%j-%H:%M:%S"),
                "CO_ppm": co_val,
                "CO2_ppm": co2_val,
                "Status": "OK"
            }
            
            # 3. Convertir a JSON y Publicar
            mensaje_json = json.dumps(payload)
            client.publish(TOPIC, mensaje_json)
            
            # 4. Feedback en consola
            print(f"📤 Enviado [{SENSOR_ID}]: CO2={co2_val}ppm | CO={co_val}ppm")
            
            # Esperar 2 segundos entre envíos
            time.sleep(2)
            
    except KeyboardInterrupt:
        print("\nSimulación detenida.")
        client.disconnect()

if __name__ == "__main__":
    run_simulator()