import streamlit as st
import paho.mqtt.client as mqtt
import json
import pandas as pd
import plotly.graph_objects as go
import time
from collections import deque
import threading

# ----------------------------------------------------------
# 1. CONFIGURACIÓN DE PÁGINA
# ----------------------------------------------------------
st.set_page_config(
    page_title="Monitor Base Lunar",
    layout="wide",
    page_icon="🌔"
)

# Constantes
BROKER = "10.42.0.1"
TOPIC = "Enviromental Sensors Network"
TARGET_ID = "A1"       # TU SENSOR (Para gráficas)
MAX_POINTS = 100       # Puntos para gráficas
MAX_LOGS = 50          # Últimos mensajes a mostrar en el log
REFRESH_RATE = 1       # Tasa de refresco

# ----------------------------------------------------------
# 2. GESTIÓN DE DATOS
# ----------------------------------------------------------
@st.cache_resource
class SensorData:
    def __init__(self):
        self.graph_data = deque(maxlen=MAX_POINTS)
        self.log_data = deque(maxlen=MAX_LOGS)
        self.lock = threading.Lock()

    def add_record(self, record):
        with self.lock:
            # 1. Log general
            self.log_data.append(record)
            
            # 2. Gráficas (Solo A1 y sin errores de temperatura)
            if record.get('ID') == TARGET_ID:
                temp = record.get("Temp_C", 0)
                # Filtro simple anti-picos de error
                if temp < 150.0: 
                    self.graph_data.append(record)

    def get_graph_df(self):
        with self.lock:
            return pd.DataFrame(list(self.graph_data))

    def get_log_df(self):
        with self.lock:
            return pd.DataFrame(list(self.log_data))

state = SensorData()

# ----------------------------------------------------------
# 3. LÓGICA MQTT
# ----------------------------------------------------------
def on_message(client, userdata, msg):
    try:
        payload = msg.payload.decode("utf-8")
        data = json.loads(payload)
        # Añadimos hora de recepción local
        data["received_at"] = time.strftime("%H:%M:%S")
        state.add_record(data)
    except Exception as e:
        print(f"Error: {e}")

@st.cache_resource
def start_mqtt():
    try:
        try:
            client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
        except AttributeError:
            client = mqtt.Client()
        client.on_message = on_message
        client.connect(BROKER, 1883, 60)
        client.subscribe(TOPIC)
        client.loop_start()
        return client
    except Exception as e:
        return None

start_mqtt()

# ----------------------------------------------------------
# 4. INTERFAZ GRÁFICA
# ----------------------------------------------------------

st.title("🛰️ Centro de Control: Red de Sensores")

# --- A. INFO (Actualizada con Presión) ---
with st.expander("ℹ️ ¿Por qué son importantes estos datos?", expanded=False):
    st.markdown("""
    En una **Base Lunar**, el control ambiental es cuestión de supervivencia. Monitorizamos:
    
    * 🌡️ **Temperatura:** Vital para mantener la homeostasis de las plantas y el confort de la tripulación.
    * 💧 **Humedad:** Esencial para la transpiración vegetal y el ciclo del agua en el hábitat cerrado.
    * ☀️ **Índice UV (UVI):** Sin atmósfera protectora, la radiación cósmica es letal. Debemos blindar el invernadero.
    * ⏲️ **Presión Atmosférica:**  Necesaria para mantener la integridad estructural del módulo y permitir el intercambio gaseoso (CO₂/O₂) de las plantas. Una caída de presión indica una fuga crítica.
    """)

st.info(f"📊 Graficando datos de: **{TARGET_ID}** | 👂 Escuchando red global: **{TOPIC}**")

# --- B. DATOS ---
df_graphs = state.get_graph_df()
df_logs = state.get_log_df()

# --- C. DESCARGA ---
if not df_graphs.empty:
    col_dl, _ = st.columns([1, 4])
    csv = df_graphs.to_csv(index=False).encode('utf-8')
    col_dl.download_button(
        label="⬇️ Descargar Mis Datos (CSV)",
        data=csv,
        file_name="mis_datos_sensor.csv",
        mime="text/csv",
        key="btn_download"
    )

st.markdown("---")

# --- D. GRÁFICAS Y MÉTRICAS ---
if not df_graphs.empty:
    latest = df_graphs.iloc[-1]

    # 1. Métricas (Ahora son 4 columnas)
    c1, c2, c3, c4 = st.columns(4)
    
    c1.metric("🌡️ Temperatura", f"{latest.get('Temp_C')} °C")
    c2.metric("💧 Humedad", f"{latest.get('Humidity_Per')} %")
    c3.metric("☀️ UVI", f"{latest.get('UVI')}")
    # Nueva métrica de Presión
    c4.metric("⏲️ Presión", f"{latest.get('Pressure_hPa', '---')} hPa")

    # Función para dibujar gráficas limpias
    def plot_metric(label, var_name, color, unit):
        if var_name in df_graphs.columns:
            fig = go.Figure()
            fig.add_trace(go.Scatter(
                x=df_graphs['received_at'], 
                y=df_graphs[var_name],
                mode='lines+markers', 
                name=label,
                line=dict(color=color, width=3)
            ))
            fig.update_layout(
                title=f"{label} ({TARGET_ID})",
                yaxis_title=unit,
                height=300,
                margin=dict(l=20, r=20, t=40, b=20),
                template="plotly_white",
                hovermode="x unified"
            )
            st.plotly_chart(fig, use_container_width=True)

    # 2. Gráficas (4 filas)
    plot_metric("Temperatura", "Temp_C", "#FF4B4B", "°C")
    plot_metric("Humedad", "Humidity_Per", "#1E90FF", "%")
    plot_metric("Índice UV", "UVI", "#9370DB", "Index")
    # Nueva gráfica de Presión (Color Verde)
    plot_metric("Presión Atmosférica", "Pressure_hPa", "#2ca02c", "hPa")

else:
    st.warning(f"⏳ Esperando datos específicos del sensor {TARGET_ID} para graficar...")

st.markdown("---")

# --- E. LOG DE MENSAJES (Limpio y Expandido) ---
st.subheader("📡 Tráfico de Red (Todos los Sensores)")

if not df_logs.empty:
    last_logs = df_logs.tail(10).iloc[::-1]
    
    for i, row in last_logs.iterrows():
        raw_dict = row.to_dict()
        # Eliminamos NaN para limpiar visualización
        clean_dict = {k: v for k, v in raw_dict.items() if pd.notna(v)}
        
        sensor_id = clean_dict.get('ID', 'Desconocido')
        time_rcv = clean_dict.get('received_at', '?')
        
        prefix = "✅" if sensor_id == TARGET_ID else "🔔"
        
        st.markdown(f"**{prefix} [{time_rcv}] ID: {sensor_id}**")
        
        # JSON completo sin contraer
        json_str = json.dumps(clean_dict, indent=4)
        st.code(json_str, language='json')

else:
    st.text("Esperando tráfico en la red...")

# ----------------------------------------------------------
# 5. ACTUALIZACIÓN AUTOMÁTICA
# ----------------------------------------------------------
time.sleep(REFRESH_RATE)
st.rerun()