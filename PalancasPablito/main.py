import subprocess
import os
import time
import sys

def main():
    print("🚀 Iniciando sistema (Método Directo)...")
    
    archivo_streamlit = "appV3.py"
    archivo_simulador = "simulacion_mensajes_externos.py"
    
    # Obtenemos la ruta exacta del Python que está ejecutando este script
    # (Ej: C:\Users\Usuario\anaconda3\python.exe)
    python_exe = sys.executable 

    if not os.path.exists(archivo_streamlit) or not os.path.exists(archivo_simulador):
        print(f"❌ Error: No se encuentra {archivo_streamlit} o {archivo_simulador}")
        input("Presiona Enter para salir...")
        return

    # COMANDO 1: STREAMLIT (Usando el módulo de Python directamente)
    # En lugar de "streamlit run", usamos "python -m streamlit run"
    cmd_streamlit = f'start "Panel Streamlit" cmd /k "{python_exe}" -m streamlit run {archivo_streamlit}'

    # COMANDO 2: SIMULADOR (Usando el mismo Python)
    cmd_simulador = f'start "Simulador B2" cmd /k "{python_exe}" {archivo_simulador}'

    print(f"1. Lanzando Streamlit con: {python_exe}...")
    subprocess.Popen(cmd_streamlit, shell=True)
    
    print("   Esperando arranque del servidor (4s)...")
    time.sleep(4)

    print(f"2. Lanzando Simulador...")
    subprocess.Popen(cmd_simulador, shell=True)

    print("\n✅ Ventanas lanzadas. Verifica que no haya errores rojos en ellas.")

if __name__ == "__main__":
    main()