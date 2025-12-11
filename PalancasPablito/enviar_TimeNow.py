import paho.mqtt.client as mqtt
import time
from datetime import datetime
import json

# Configuración del broker MQTT
broker = "broker.emqx.io"  # Dirección de tu broker MQTT
port = 1883  # Puerto MQTT
topic = "TimeNow"  # Tópico MQTT para publicar el timestamp

# Crear cliente MQTT
client = mqtt.Client()

# Función para conectar al broker MQTT
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Conectado a MQTT broker")
    else:
        print("Error de conexión MQTT, código:", rc)

client.on_connect = on_connect

# Conectar al broker MQTT
client.connect(broker, port, 60)

# Función para obtener el timestamp en formato "YYYY-DDD-HH:MM:SS"
def get_timestamp():
    now = datetime.now()
    # Obtener el día del año (DOY)
    doy = now.timetuple().tm_yday
    return f"{now.year}-{doy:03d}-{now.strftime('%H:%M:%S')}"

# Publicar el timestamp cada 10 segundos
while True:
    timestamp = get_timestamp()  # Obtener el timestamp actual
    payload = {
        "TimeNow": timestamp
    }
    payload_json = json.dumps(payload)  # Convertir el diccionario a JSON

    # Publicar el timestamp en el tópico MQTT
    client.publish(topic, payload_json)
    print(f"Publicado: {payload_json}")  # Mostrar en consola el timestamp

    client.loop()  # Asegurarse de que el cliente MQTT esté en bucle

    time.sleep(5)  # Esperar 10 segundos antes de la siguiente publicación
