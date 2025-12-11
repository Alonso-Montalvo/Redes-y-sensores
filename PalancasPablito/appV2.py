import streamlit as st
import paho.mqtt.client as mqtt
import json
import pandas as pd
import plotly.graph_objects as go
import time
from collections import deque
import threading

# ----------------------------------------------------------
# CONFIGURACIÓN Y CONSTANTES
# ----------------------------------------------------------
st.set_page_config(
    page_title="Monitor Ambiental MQTT",
    layout="wide",
    page_icon="🌡️"
)

BROKER = "broker.emqx.io"
TOPIC = "Enviromental Sensors Network"
TARGET_ID = "A1"
MAX_POINTS = 200
UPDATE_INTERVAL_SECONDS = 5 # Intervalo para la comprobación del estado

# Inicializar st.session_state para la gestión de la actualización
if 'new_data' not in st.session_state:
    st.session_state.new_data = False

# Diccionario de variables (estático)
variables_map = {
    "Temperatura y Humedad": ["Temp_C", "Humidity_Per"],
    "Solo Temperatura": ["Temp_C"],
    "Solo Humedad": ["Humidity_Per"],
    "Solo UVI": ["UVI"],
    "Temperatura, Humedad y UVI": ["Temp_C", "Humidity_Per", "UVI"],
    "Todas las variables": ["Temp_C", "Humidity_Per", "UVI", "Pressure_hPa"]
}

# ----------------------------------------------------------
# ESTADO COMPARTIDO Y LÓGICA MQTT
# ----------------------------------------------------------
@st.cache_resource
class SharedState:
    """Clase para mantener los datos de forma persistente y concurrente."""
    def __init__(self):
        self.data = deque(maxlen=MAX_POINTS)
        self.lock = threading.Lock() # Uso de un Lock para seguridad en el acceso

    def add_record(self, record):
        with self.lock:
            self.data.append(record)

    def get_dataframe(self):
        with self.lock:
            if not self.data: 
                return pd.DataFrame()
            return pd.DataFrame(list(self.data))

state = SharedState()

def on_message(client, userdata, msg):
    try:
        payload = msg.payload.decode("utf-8")
        data = json.loads(payload)
        
        if 'ID' in data and data['ID'] == TARGET_ID:
             data["received_at"] = time.strftime("%H:%M:%S")
             state.add_record(data)
             
             # **ESTA ES LA CLAVE:** Marcar que hay nuevos datos y forzar la recarga
             # Usamos una función de callback para evitar errores de contexto.
             if st.session_state.get('app_running', False):
                 st.session_state.new_data = True
                 print("Nuevo mensaje recibido, programando rerun...")
                 # No usamos st.rerun() directamente en el hilo MQTT.
                 # El bucle del timer lo detectará.

    except Exception as e:
        print(f"Error procesando mensaje: {e}")

@st.cache_resource
def start_mqtt_client():
    # Usando CallbackAPIVersion.VERSION2 para evitar DeprecationWarning
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2) 
    client.on_message = on_message
    client.connect(BROKER, 1883, 60)
    client.subscribe(TOPIC)
    client.loop_start() 
    return client

client = start_mqtt_client()

def mqtt_status(broker):
    try:
        temp_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
        temp_client.connect(broker, 1883, 2)
        temp_client.disconnect()
        return True
    except: return False

# ----------------------------------------------------------
# LÓGICA DE VISUALIZACIÓN
# ----------------------------------------------------------

# Configurar el estado de la aplicación como en ejecución
st.session_state.app_running = True

# 1. Título y explicación
st.title("🌡️ Monitoreo de Temperatura y Humedad en una Base Lunar")

with st.expander("¿Por qué son importantes estos datos?"):
    st.markdown(
        """
        <span style='font-size:16px;'>
        Los parámetros como la <b>temperatura</b>, <b>humedad</b> y el <b>índice UV (UVI)</b> son cruciales para el desarrollo y crecimiento de las plantas en ambientes controlados, como los que se necesitan en una base lunar. [Image of Controlled Environment Agriculture in Space] El control preciso de estos factores ayuda a prevenir riesgos y optimizar el ambiente para la producción de oxígeno y alimentos.
        </span>
        """,
        unsafe_allow_html=True
    )

# 2. Selector
opcion = st.selectbox(
    "¿Qué datos quieres visualizar?",
    variables_map.keys(),
    key="var_selector_unique"
)
cols_to_display = variables_map.get(opcion, [])

st.markdown("---") 

# Obtener datos y filtrar (Se ejecuta en cada rerun)
df = state.get_dataframe()
if not df.empty and 'ID' in df.columns:
    df_filtered = df[df['ID'] == TARGET_ID].copy()
else:
    df_filtered = pd.DataFrame()

# 3. Estado de Conexión
if mqtt_status(BROKER):
    st.success("🟢 Conectado al broker MQTT")
else:
    st.error("🔴 Desconectado del broker MQTT")

# 4. Métricas
st.header("Métricas Actuales")
if not df_filtered.empty:
    latest = df_filtered.iloc[-1]
    metric_cols = st.columns(len(cols_to_display))
    for i, var in enumerate(cols_to_display):
        valor = latest.get(var, None)
        if var == "Temp_C": metric_cols[i].metric("Temperatura", f"{valor} °C")
        elif var == "Humidity_Per": metric_cols[i].metric("Humedad", f"{valor} %")
        elif var == "UVI": metric_cols[i].metric("Rayos UV (UVI)", f"{valor}")
        elif var == "Pressure_hPa": metric_cols[i].metric("Presión", f"{valor} hPa")
else:
    st.info("Esperando los primeros datos del sensor...")

st.markdown("---") 

# 5. Gráficos
st.header(f"Lecturas en tiempo real – ID {TARGET_ID}")
if cols_to_display and not df_filtered.empty:
    chart_data = df_filtered[cols_to_display].copy()
    
    # Lógica de Plotly
    fig = go.Figure()
    for var in cols_to_display:
        if var in chart_data:
            fig.add_trace(go.Scatter(
                x=chart_data.index, y=chart_data[var], mode="lines",
                name=var.replace("_", " "), line=dict(width=3)
            ))
    fig.update_layout(height=500, template="simple_white")
    st.plotly_chart(fig, use_container_width=True)
else:
     st.info("Selecciona variables o espera datos para mostrar el gráfico.")

st.markdown("---") 

# 6. Datos en Crudo (Punto 2: Revertido a visualización original organizada)
if not df_filtered.empty:
    st.subheader("Últimos 10 mensajes recibidos:")
    # Usamos st.code para formatear el JSON, manteniendo la organización
    for record in df_filtered.tail(10).to_dict(orient='records'):
        st.code(json.dumps(record, indent=2), language='json')
else:
    st.text("No se han recibido datos para mostrar.")

# 7. Botón de Descarga (¡NO en el bucle!)
if not df_filtered.empty:
    csv_data = df_filtered.to_csv(index=False).encode('utf-8')
    st.download_button(
        label="Descargar datos en formato CSV",
        data=csv_data,
        file_name="datos_sensor_A1.csv",
        mime="text/csv",
        # Clave ÚNICA que no está en un bucle y no da error
        key="download_button_final" 
    )

# ----------------------------------------------------------
# MANEJO DE ACTUALIZACIÓN AUTOMÁTICA (El reemplazo del While True)
# ----------------------------------------------------------

# Función para reiniciar el script de Streamlit si hay nuevos datos
def check_for_data_and_rerun():
    # Solo ejecutar si hay nuevos datos
    if st.session_state.new_data:
        st.session_state.new_data = False # Resetear la bandera
        st.rerun() # Forzar la re-ejecución del script

    # Programar la próxima comprobación
    threading.Timer(UPDATE_INTERVAL_SECONDS, check_for_data_and_rerun).start()

# Asegurar que la comprobación solo se inicie una vez
if 'rerun_timer_started' not in st.session_state:
    check_for_data_and_rerun()
    st.session_state.rerun_timer_started = True

# La aplicación Streamlit termina aquí. El timer se encarga de re-ejecutarla
# cuando hay nuevos datos o cada 5 segundos si quieres una actualización base.