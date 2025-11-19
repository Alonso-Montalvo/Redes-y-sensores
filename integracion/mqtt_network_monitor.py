import paho.mqtt.client as mqtt
import json
from datetime import datetime

# --- Configuracion ---
BROKER_HOST = "broker.emqx.io"
BROKER_PORT = 1883
TOPIC_NETWORK = "Enviromental Sensors Network"
MY_SENSOR_ID = "Sensor_Astro_ID_007" # ¡DEBES USAR EL MISMO ID QUE EN EL ARDUINO!

# --- Funciones de Callback ---

def on_connect(client, userdata, flags, rc):
    print(f"Conectado al broker MQTT con resultado: {rc}")
    client.subscribe(TOPIC_NETWORK)
    print(f"Suscrito a la red de sensores: {TOPIC_NETWORK}")

def on_message(client, userdata, msg):
    try:
        # Decodificar el payload y parsear el JSON
        payload = msg.payload.decode()
        data = json.loads(payload)
        
        sensor_id = data.get("sensor_id")
        timestamp = data.get("timestamp_unix")
        
        # Convertir timestamp a hora legible
        hora_legible = datetime.fromtimestamp(timestamp).strftime('%Y-%m-%d %H:%M:%S')
        
        # --- Punto 2: Filtrar y mostrar solo tu sensor ---
        if sensor_id == MY_SENSOR_ID:
            print(f"\n✅ --- MIS DATOS RECIBIDOS ({MY_SENSOR_ID}) ---")
            print(f"  Hora: {hora_legible}")
            print(f"  Localización: {data.get('location', 'N/A')}")
            # Muestra el dato de tu sensor (ej. nivel de oxígeno)
            if 'data' in data and 'oxigen_percent' in data['data']:
                print(f"  Nivel de Oxígeno: {data['data']['oxigen_percent']}%")
        
        # --- Punto 3: Mostrar datos de otros sensores (Ampliación) ---
        else:
            print(f"\n➡️ --- DATOS DE OTRO SENSOR ({sensor_id}) ---")
            print(f"  Hora: {hora_legible}")
            # Muestra todos los datos del sensor desconocido
            print(f"  Payload Completo: {data}")


    except json.JSONDecodeError:
        print(f"Error: Payload no es un JSON válido: {msg.payload.decode()}")
    except Exception as e:
        print(f"Error inesperado al procesar mensaje: {e}")

# --- Programa Principal ---

client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1, "PythonNetworkMonitor")
client.on_connect = on_connect
client.on_message = on_message

try:
    print(f"Intentando conectar a {BROKER_HOST}:{BROKER_PORT}...")
    client.connect(BROKER_HOST, BROKER_PORT, 60)
except Exception as e:
    print(f"Error al conectar: {e}")
    exit()

client.loop_forever()