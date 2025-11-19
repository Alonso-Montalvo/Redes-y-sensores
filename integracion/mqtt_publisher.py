import paho.mqtt.client as mqtt
import time

# -----------------
# CONFIGURACIÓN
# -----------------
MQTT_BROKER = "broker.emqx.io"
MQTT_TOPIC = "test1"
CLIENT_ID = "Python_Publisher"

# -----------------
# FUNCIONES DE CALLBACK
# -----------------

def on_connect(client, userdata, flags, rc):
    """Callback que se llama cuando el cliente se conecta al broker."""
    if rc == 0:
        print(f"✅ Conectado al broker MQTT: {MQTT_BROKER}")
        print("--- Listo para enviar mensajes ---")
    else:
        print(f"❌ Fallo en la conexión, código de retorno: {rc}")

# -----------------
# LÓGICA PRINCIPAL
# -----------------
client = mqtt.Client(client_id=CLIENT_ID)
client.on_connect = on_connect

try:
    # Intento de conexión
    client.connect(MQTT_BROKER, 1883, 60)
    
    # Iniciar un hilo de fondo para gestionar las comunicaciones de red
    client.loop_start() 

    # Bucle de envío de mensajes
    while True:
        message = input(f"Escribe un mensaje para '{MQTT_TOPIC}' (o 'exit' para salir): ")
        if message.lower() == 'exit':
            break

        # Publicar el mensaje
        client.publish(MQTT_TOPIC, message)
        print(f"🚀 Enviado: '{message}'")
        time.sleep(0.1) # Pequeña pausa
        
except Exception as e:
    print(f"Un error ocurrió: {e}")

finally:
    # Desconectar y limpiar el hilo de fondo
    client.loop_stop()
    client.disconnect()
    print("Desconectado del broker.")