import paho.mqtt.client as mqtt
import time
from datetime import datetime

# --- Configuracion ---
BROKER_HOST = "broker.emqx.io"
BROKER_PORT = 1883
TOPIC_TIME = "TimeNow"

# --- Variables Globales ---
last_update_time = None

# --- Funciones de Callback ---

def on_connect(client, userdata, flags, rc):
    print(f"Conectado al broker MQTT con resultado: {rc}")
    client.subscribe(TOPIC_TIME)
    print(f"Suscrito al topic: {TOPIC_TIME}")

def on_message(client, userdata, msg):
    global last_update_time
    
    try:
        # Asumimos que el payload es una marca de tiempo (timestamp) UNIX en string
        timestamp_str = msg.payload.decode()
        unix_timestamp = int(timestamp_str)
        
        # Convertir el timestamp UNIX a un objeto datetime
        current_dt = datetime.fromtimestamp(unix_timestamp)
        
        # Guardar la hora de la última actualización
        last_update_time = current_dt
        
        print(f"\n--- HORA RECIBIDA ---")
        print(f"Timestamp UNIX: {unix_timestamp}")
        print(f"Hora Decodificada: {current_dt.strftime('%Y-%m-%d %H:%M:%S')}")

    except ValueError:
        print(f"Error: Payload no es un timestamp UNIX válido: {msg.payload.decode()}")
    except Exception as e:
        print(f"Error inesperado al procesar mensaje: {e}")

# --- Programa Principal ---

client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1, "PythonTimeSubscriber")
client.on_connect = on_connect
client.on_message = on_message

try:
    print(f"Intentando conectar a {BROKER_HOST}:{BROKER_PORT}...")
    client.connect(BROKER_HOST, BROKER_PORT, 60)
except Exception as e:
    print(f"Error al conectar: {e}")
    exit()

# Inicia un hilo en segundo plano para manejar la red (no bloqueante)
client.loop_start()

# Bucle principal para mostrar la hora cada 10s
try:
    last_display_time = time.time() - 10 # Para forzar la primera impresión
    
    while True:
        current_time = time.time()
        
        if (current_time - last_display_time) >= 10:
            last_display_time = current_time
            
            if last_update_time:
                print(f"[{datetime.now().strftime('%H:%M:%S')}] Hora actual del sistema (basada en MQTT): {last_update_time.strftime('%H:%M:%S')}")
            else:
                print(f"[{datetime.now().strftime('%H:%M:%S')}] Esperando la primera marca de tiempo de TimeNow...")
                
        time.sleep(1) # Espera 1 segundo para no saturar el CPU
        
except KeyboardInterrupt:
    print("\nPrograma detenido por el usuario.")
    
client.loop_stop()
client.disconnect()