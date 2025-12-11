import streamlit as st
st.markdown(
    """
    <style>
    body, .stApp {
        background-color: #f5e9da;
        color: #6e4b1b;
    }
    .stButton>button, .stTextInput>div>input, .stSelectbox>div>div {
        background-color: #e6d3b3;
        color: #6e4b1b;
        border-radius: 8px;
        border: 1px solid #bfa77a;
    }
    .stDataFrame, .stTable {
        background-color: #f5e9da !important;
        color: #6e4b1b !important;
    }
    h1, h2, h3, h4, h5, h6 {
        color: #8d6e3b;
    }
    </style>
    """,
    unsafe_allow_html=True
)
import paho.mqtt.client as mqtt
import json
import pandas as pd
import plotly.graph_objects as go
import time
from collections import deque

# ----------------------------------------------------------
# CONFIGURACIÓN DE LA PÁGINA
# ----------------------------------------------------------
st.set_page_config(
    page_title="Monitor Ambiental MQTT",
    layout="wide",
    page_icon="🌡️"
)

# ----------------------------------------------------------
# CONSTANTES Y ESTADO GLOBAL
# ----------------------------------------------------------
BROKER = "192.168.0.88"
TOPIC = "Enviromental Sensors Network"
TARGET_ID = "A1"
MAX_POINTS = 200

# Usamos st.cache_resource para mantener una única instancia de los datos
# y del cliente MQTT, evitando que se reinicien cada vez que Streamlit refresca.
@st.cache_resource
class SharedState:
    def __init__(self):
        self.data = deque(maxlen=MAX_POINTS)
        self.lock = False # Simple flag por si acaso

    def add_record(self, record):
        self.data.append(record)
    
    def get_dataframe(self):
        return pd.DataFrame(list(self.data))

# Instancia compartida de datos
state = SharedState()

# ----------------------------------------------------------
# LÓGICA MQTT
# ----------------------------------------------------------
def on_message(client, userdata, msg):
    try:
        payload = msg.payload.decode("utf-8")
        data = json.loads(payload)
        # Guardar todos los mensajes recibidos, sin filtrar por ID
        data["received_at"] = time.strftime("%H:%M:%S")
        state.add_record(data)
    except Exception as e:
        print(f"Error procesando mensaje: {e}")

@st.cache_resource
def start_mqtt_client():
    client = mqtt.Client()
    client.on_message = on_message
    client.connect(BROKER, 1883, 60)
    client.subscribe(TOPIC)
    client.loop_start()  # Ejecuta en un hilo en segundo plano
    return client

# Iniciar el cliente MQTT (solo se ejecuta una vez gracias al cache)
start_mqtt_client()

# ----------------------------------------------------------
# INTERFAZ DE STREAMLIT
# Estado de conexión MQTT
def mqtt_status():
    try:
        client = mqtt.Client()
        client.connect(BROKER, 1883, 2)
        client.disconnect()
        return True
    except:
        return False

status = mqtt_status()
if status:
    st.success("🟢 Conectado al broker MQTT")
else:
    st.error("🔴 Desconectado del broker MQTT")
# ----------------------------------------------------------

# Nuevo título
st.title("🌡️ Monitoreo de Temperatura y Humedad")

# Descripción breve sobre los datos

# Explicación en expander
with st.expander("¿Por qué son interesantes estos datos?"):
    st.markdown(
        """
        <style>
        .streamlit-expanderContent {
            background-color: #f7f2eb !important;
        }
        </style>
        <span style='font-size:16px;'>
        <b>Temperatura</b> y <b>humedad</b> ambiental son fundamentales para el confort, la salud y el funcionamiento de equipos electrónicos. Un monitoreo adecuado ayuda a prevenir riesgos, optimizar ambientes y detectar anomalías en tiempo real.<br><br>
        <b>Rayos UV (UVI)</b> son importantes para la salud, ya que una exposición elevada puede causar daños en la piel y aumentar el riesgo de enfermedades. Medir el índice UV permite tomar decisiones informadas para protegerse.<br><br>
        Este panel te permite visualizar y analizar estos parámetros en tiempo real, facilitando la gestión ambiental y la prevención de riesgos.
        </span>
        """,
        unsafe_allow_html=True
    )

# Selector para elegir qué datos mostrar
opcion = st.selectbox(
    "¿Qué datos quieres visualizar?",
    ("Temperatura y Humedad", "Solo Temperatura", "Solo Humedad", "Solo UVI", "Temperatura, Humedad y UVI", "Todas las variables")
)

# Contenedores para métricas y gráficos
placeholder_metrics = st.empty()
placeholder_charts = st.empty()

# --- VISUALIZACIÓN REACTIVA STREAMLIT ---
df = state.get_dataframe()
if 'ID' in df.columns:
    df = df[df['ID'] == 'A1']

variables = {
    "Temperatura y Humedad": ["Temp_C", "Humidity_Per"],
    "Solo Temperatura": ["Temp_C"],
    "Solo Humedad": ["Humidity_Per"],
    "Solo UVI": ["UVI"],
    "Temperatura, Humedad y UVI": ["Temp_C", "Humidity_Per", "UVI"],
    "Todas las variables": ["Temp_C", "Humidity_Per", "UVI", "Pressure_hPa"]
}
cols = variables.get(opcion, [])

# Mostrar métricas actuales
if not df.empty:
    latest = df.iloc[-1]
    metric_cols = st.columns(len(cols))
    for i, var in enumerate(cols):
        valor = latest.get(var, None)
        if var == "Temp_C":
            metric_cols[i].metric("Temperatura", f"{valor} °C")
        elif var == "Humidity_Per":
            metric_cols[i].metric("Humedad", f"{valor} %")
        elif var == "UVI":
            metric_cols[i].metric("Rayos UV (UVI)", f"{valor}")

# Mostrar gráfica en tiempo real
if cols and not df.empty:
    chart_data = df[cols].copy()
    x = chart_data.index  # Eje X: número de muestra, igual que en monitor_qtt.py

    ylabels = {
        "Temp_C": "Temperatura (°C)",
        "Humidity_Per": "Humedad (%)",
        "UVI": "UVI",
        "Pressure_hPa": "Presión (hPa)"
    }
    color_map = {
        "Temp_C": "#e67300",
        "Humidity_Per": "#0099cc",
        "UVI": "#a020f0",
        "Pressure_hPa": "#228B22"
    }
    line_widths = {
        "Temp_C": 3,
        "Humidity_Per": 3,
        "UVI": 3,
        "Pressure_hPa": 3
    }
    fig = go.Figure()
    for var in cols:
        if var in chart_data:
            fig.add_trace(go.Scatter(
                x=x,
                y=chart_data[var],
                mode="lines",
                name=ylabels.get(var, var),
                line=dict(color=color_map.get(var, None), width=line_widths.get(var, 2))
            ))
    fig.update_layout(
        xaxis_title="Muestras (tiempo)",
        yaxis_title="Valor medido",
        title="Lecturas en tiempo real – ID A1",
        legend_title="Variable",
        template="simple_white",
        height=500,
        legend=dict(orientation="h", yanchor="bottom", y=1.02, xanchor="right", x=1),
        margin=dict(l=40, r=20, t=40, b=40)
    )
    st.plotly_chart(fig, use_container_width=True)
elif not df.empty:
    st.info("No hay datos suficientes para graficar la selección actual.")
else:
    st.warning("No se han recibido datos aún.")