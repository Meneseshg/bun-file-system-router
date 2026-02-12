#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>
#include <ctime>
#include <iomanip>
#include <cstdlib>
#include <sstream>

// Rutas
const char *CONFIG_FILE = "camera.url";
const char *SENSOR_27_FILE = "/dev/shm/pin_27_state";
const char *FFMPEG_PID_FILE = "/dev/shm/ffmpeg.pid";

// Función mejorada para extraer valores específicos del archivo de configuración
std::string get_config_value(const std::string &key)
{
    std::ifstream file(CONFIG_FILE);
    std::string line;
    if (file.is_open())
    {
        while (std::getline(file, line))
        {
            // Buscamos si la línea comienza con el prefijo solicitado (ej: "RTSP=")
            if (line.find(key + "=") == 0)
            {
                return line.substr(key.length() + 1); // Retorna lo que está después del '='
            }
        }
        file.close();
    }
    return "";
}

int read_sensor_state(const char *filePath)
{
    std::ifstream file(filePath);
    char state;
    if (file.is_open())
    {
        if (file >> state)
        {
            file.close();
            return (state == '1') ? 1 : 0;
        }
        file.close();
    }
    return -1;
}

// Genera un nombre de archivo basado en la fecha y hora actual
std::string get_timestamp_filename()
{
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << "video_" << std::put_time(std::localtime(&in_time_t), "%Y%m%d_%H%M%S") << ".mp4";
    return ss.str();
}

int main()
{
    // Extraemos solo la URL de video
    std::string rtsp_url = get_config_value("RTSP");

    if (rtsp_url.empty())
    {
        std::cerr << "[ERROR] No se encontró la etiqueta RTSP= en camera.url" << std::endl;
        return 1;
    }

    bool is_recording = false;
    std::cout << "--- Camera Manager v3.1: Configuración Dual ---" << std::endl;
    std::cout << "[CONFIG] Stream detectado: " << rtsp_url << std::endl;

    while (true)
    {
        int sensor = read_sensor_state(SENSOR_27_FILE);

        if (sensor == 1 && !is_recording)
        {
            std::string filename = get_timestamp_filename();
            std::cout << "[VIDEO] >>> Grabando: " << filename << std::endl;

            // Usamos la URL extraída dinámicamente
            std::string cmd = "ffmpeg -y -rtsp_transport tcp -i \"" + rtsp_url +
                              "\" -c copy -movflags +faststart " + filename +
                              " > /dev/null 2>&1 & echo $! > " + FFMPEG_PID_FILE;

            std::system(cmd.c_str());
            is_recording = true;
        }
        else if (sensor == 0 && is_recording)
        {
            std::cout << "[VIDEO] <<< Deteniendo grabacion de forma limpia..." << std::endl;

            // Usamos -2 (SIGINT) para que FFmpeg cierre el archivo .mp4 correctamente
            std::string stop_cmd = "kill -2 $(cat " + std::string(FFMPEG_PID_FILE) + ") 2>/dev/null";
            std::system(stop_cmd.c_str());

            // Damos un pequeño margen para que termine de escribir antes de borrar el PID
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            std::string rm_pid = "rm " + std::string(FFMPEG_PID_FILE) + " 2>/dev/null";
            std::system(rm_pid.c_str());

            is_recording = false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    return 0;
}