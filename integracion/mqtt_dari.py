import paho.mqtt.client as mqtt
import time
import threading

# Configuración MQTT
MQTT_BROKER = "10.42.0.1"
MQTT_PORT = 1883
MQTT_TOPIC = "test1"

class MQTTClient:
    def __init__(self):
        self.client = mqtt.Client()
        self.setup_callbacks()
       
    def setup_callbacks(self):
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        self.client.on_publish = self.on_publish
       
    def on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            print(f"✅ Conectado exitosamente al broker: {MQTT_BROKER}")
            client.subscribe(MQTT_TOPIC)
            print(f"📡 Suscrito al tópico: {MQTT_TOPIC}")
        else:
            print(f"❌ Error de conexión. Código: {rc}")
           
    def on_message(self, client, userdata, msg):
        mensaje = msg.payload.decode()
        print(f"\n📨 MENSAJE RECIBIDO:")
        print(f"   Tópico: {msg.topic}")
        print(f"   Texto: {mensaje}")
        print(f"   Hora: {time.strftime('%H:%M:%S')}")
        print("-" * 40)
       
    def on_publish(self, client, userdata, mid):
        print(f"📤 Mensaje publicado con ID: {mid}")
       
    def enviar_mensaje(self, mensaje):
        result = self.client.publish(MQTT_TOPIC, mensaje)
        if result.rc == mqtt.MQTT_ERR_SUCCESS:
            print(f"✍️  Mensaje enviado: '{mensaje}'")
        else:
            print(f"❌ Error al enviar mensaje")
           
    def iniciar_recepcion(self):
        try:
            print(f"🔌 Conectando a {MQTT_BROKER}:{MQTT_PORT}...")
            self.client.connect(MQTT_BROKER, MQTT_PORT, 60)
           
            # Iniciar loop en segundo plano para recibir mensajes
            self.client.loop_start()
            print("🔄 Listo para enviar y recibir mensajes...")
            print("   Escribe tu mensaje y presiona Enter (o 'quit' para salir)")
           
        except Exception as e:
            print(f"❌ Error: {e}")
           
    def detener(self):
        self.client.loop_stop()
        self.client.disconnect()
        print("🔌 Desconectado del broker MQTT")

def interfaz_usuario(mqtt_client):
    while True:
        try:
            mensaje = input().strip()
           
            if mensaje.lower() in ['quit', 'exit', 'salir', 'q']:
                break
               
            if mensaje:
                mqtt_client.enviar_mensaje(mensaje)
               
        except KeyboardInterrupt:
            break
        except Exception as e:
            print(f"❌ Error: {e}")

def main():
    mqtt_client = MQTTClient()
    mqtt_client.iniciar_recepcion()
   
    try:
        interfaz_usuario(mqtt_client)
    finally:
        mqtt_client.detener()

if __name__ == "__main__":
    main()

