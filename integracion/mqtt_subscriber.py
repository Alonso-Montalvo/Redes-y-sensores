import paho.mqtt.client as mqtt

# -----------------
# CONFIGURACIÓN
# -----------------
MQTT_BROKER = "broker.emqx.io"
MQTT_TOPIC = "test1"
CLIENT_ID = "Python_Subscriber"

# -----------------
# FUNCIONES DE CALLBACK
# -----------------

def on_connect(client, userdata, flags, rc):
    """Callback que se llama cuando el cliente se conecta al broker."""
    if rc == 0:
        print(f"✅ Conectado al broker MQTT: {MQTT_BROKER}")
        # Suscribirse al topic después de la conexión
        client.subscribe(MQTT_TOPIC)
        print(f"👂 Suscrito al topic: {MQTT_TOPIC}")
    else:
        print(f"❌ Fallo en la conexión, código de retorno: {rc}")

def on_message(client, userdata, msg):
    """Callback que se llama cuando se recibe un mensaje del broker."""
    # Decodificar el payload de bytes a string (UTF-8)
    payload = msg.payload.decode()
    print(f"\n📨 Mensaje Recibido en [{msg.topic}]: {payload}")

# -----------------
# LÓGICA PRINCIPAL
# -----------------
client = mqtt.Client(client_id=CLIENT_ID)
client.on_connect = on_connect
client.on_message = on_message

try:
    # Intento de conexión
    client.connect(MQTT_BROKER, 1883, 60)
    
    # Bucle de procesamiento de red (bloquea el hilo, esperando mensajes)
    client.loop_forever()

except Exception as e:
    print(f"Un error ocurrió durante la conexión o el loop: {e}")