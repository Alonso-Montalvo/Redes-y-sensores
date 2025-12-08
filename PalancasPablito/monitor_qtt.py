import json
import paho.mqtt.client as mqtt
import matplotlib.pyplot as plt
from collections import deque
import threading
import time

# ----------------------------------------------------------
# CONFIGURACIÓN
# ----------------------------------------------------------
BROKER = "broker.emqx.io"
TOPIC = "Enviromental Sensors Network"
TARGET_ID = "A1"

MAX_POINTS = 200

time_buf = deque(maxlen=MAX_POINTS)
temp_buf = deque(maxlen=MAX_POINTS)
hum_buf = deque(maxlen=MAX_POINTS)
pres_buf = deque(maxlen=MAX_POINTS)
uvi_buf = deque(maxlen=MAX_POINTS)

# ----------------------------------------------------------
# MQTT CALLBACKS
# ----------------------------------------------------------
def on_connect(client, userdata, flags, rc):
    print("Conectado al broker:", rc)
    client.subscribe(TOPIC)


def on_message(client, userdata, msg):
    raw = msg.payload.decode("utf-8").strip()
    print("Mensaje recibido:", raw)

    try:
        data = json.loads(raw)
    except:
        return

    # Filtrar ID
    if data.get("ID") != TARGET_ID:
        return

    # Extraer datos reales del proyecto
    t = data.get("Tiempo_UTC", time.time())
    temp = float(data.get("Temp_C", 0))
    hum = float(data.get("Humidity_Per", 0))
    pres = float(data.get("Pressure_hPa", 0))
    uvi = float(data.get("UVI", 0))

    # Añadir al buffer
    time_buf.append(t)
    temp_buf.append(temp)
    hum_buf.append(hum)
    pres_buf.append(pres)
    uvi_buf.append(uvi)


# ----------------------------------------------------------
# HILO MQTT
# ----------------------------------------------------------
def start_mqtt():
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(BROKER, 1883, 60)
    client.loop_forever()

# Lanzar MQTT en segundo plano
threading.Thread(target=start_mqtt, daemon=True).start()

# ----------------------------------------------------------
# GRAFICADO EN TIEMPO REAL
# ----------------------------------------------------------
plt.ion()
fig, ax = plt.subplots()

line_temp, = ax.plot([], [], label="Temperatura (°C)")
line_hum,  = ax.plot([], [], label="Humedad (%)")
line_pres, = ax.plot([], [], label="Presión (hPa)")
line_uvi,  = ax.plot([], [], label="UVI")

ax.set_xlabel("Muestras (tiempo)")
ax.set_title("Lecturas en tiempo real – ID A1")
ax.legend()

# Loop principal
while True:
    if len(time_buf) > 0:
        line_temp.set_data(range(len(temp_buf)), temp_buf)
        line_hum.set_data(range(len(hum_buf)), hum_buf)
        line_pres.set_data(range(len(pres_buf)), pres_buf)
        line_uvi.set_data(range(len(uvi_buf)), uvi_buf)

        ax.relim()
        ax.autoscale_view()
        plt.pause(0.5)

    time.sleep(0.1)
