#!/usr/bin/env python3
import subprocess
import logging
import sys
import time

# --- CONFIGURACIÓN ---
RTSP_URL = "rtsp://admin:Flexo900742055!@192.168.1.199:554/cam/realmonitor?channel=1&subtype=0"
OUTPUT_FILENAME = "grabacion_rtsp.mp4"
# ---------------------

logging.basicConfig(level=logging.INFO, format='%(asctime)s [%(levelname)s] %(message)s')

def main():
    logging.info("Iniciando grabación... (Usando timeout de red)")

    # Reordenamos los argumentos: Las opciones de entrada DEBEN ir antes de -i
    cmd = [
        'ffmpeg', '-y',
        '-rtsp_transport', 'tcp',
        '-timeout', '5000000',  # 5 segundos en microsegundos
        '-i', RTSP_URL,
        '-an',
        '-c:v', 'copy',
        '-map', '0',
        '-movflags', '+faststart',
        OUTPUT_FILENAME
    ]

    # Ejecutamos FFmpeg. 
    # Usamos stderr=subprocess.STDOUT para que los errores de FFmpeg salgan en nuestra consola
    proceso = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=sys.stdout, stderr=sys.stderr, text=True)
    
    logging.info(f"Grabando en {OUTPUT_FILENAME}...")
    logging.info("Presiona CTRL+C para detener.")

    try:
        while True:
            if proceso.poll() is not None:
                logging.error(f"FFmpeg se detuvo. Código de salida: {proceso.poll()}")
                break
            time.sleep(1)
            
    except KeyboardInterrupt:
        logging.info("\nDeteniendo grabación...")
        try:
            # Enviamos 'q' para que FFmpeg cierre bien el MP4
            proceso.communicate(input='q', timeout=3)
        except:
            proceso.terminate()
        
    finally:
        logging.info("Proceso finalizado.")
        print("\n" + "="*40)
        print("TERMINADO. Presiona CTRL+C para salir.")
        print("="*40)
        try:
            while True: time.sleep(1)
        except KeyboardInterrupt:
            pass

if __name__ == "__main__":
    main()