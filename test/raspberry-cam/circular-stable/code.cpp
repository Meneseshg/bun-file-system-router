#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>
#include <vector>
#include <ctime>
#include <iomanip>
#include <cstdlib>
#include <sstream>
#include <set>
#include <algorithm>
#include <sys/stat.h> // Para crear carpetas
#include <cstdio>     // Para remove y rename

#include <dirent.h>   // Para leer directorios

// --- Configuración de Rutas ---
const char *CONFIG_FILE = "camera.url";
const char *SENSOR_27_FILE = "/dev/shm/pin_27_state";
const char *RAM_PATH = "/dev/shm/";
const std::string OUTPUT_FOLDER = "grabaciones/";

// Función para asegurar que la carpeta de destino exista
void ensure_output_folder()
{
    struct stat info;
    if (stat(OUTPUT_FOLDER.c_str(), &info) != 0)
    {
        std::cout << "[SISTEMA] Creando carpeta: " << OUTPUT_FOLDER << std::endl;
        std::system(("mkdir -p " + OUTPUT_FOLDER).c_str());
    }
}

// Función para extraer valores del archivo de configuración
std::string get_config_value(const std::string &key)
{
    std::ifstream file(CONFIG_FILE);
    std::string line;
    if (file.is_open())
    {
        while (std::getline(file, line))
        {
            if (line.find(key + "=") == 0)
                return line.substr(key.length() + 1);
        }
    }
    return "";
}

// Hilo del Buffer Persistente
void run_buffer_thread(std::string rtsp_url)
{
    // Mantiene los fragmentos en RAM de forma LINEAL (sin wrap, seg00000.ts, seg00001.ts...)
    // Esto garantiza que los archivos nunca se sobrescriban.
    // Nosotros borraremos los viejos manualmente.
    std::string buffer_cmd = "ffmpeg -hide_banner -loglevel error -rtsp_transport tcp -i \"" + rtsp_url +
                             "\" -c copy -an -f segment -segment_time 1 -segment_list " + RAM_PATH + "buffer.m3u8 " +
                             "-segment_list_size 300 -flags +global_header -segment_list_flags +live " +
                             RAM_PATH + "seg%05d.ts";

    while (true)
    {
        std::cout << "[BUFFER] Conectando a cámara (Modo Lineal Seguro)..." << std::endl;
        std::system(buffer_cmd.c_str());
        std::this_thread::sleep_for(std::chrono::seconds(5)); // Reintento en caso de caída
    }
}

// Función de limpieza (Garbage Collector)
// Borra archivos .ts que sean más viejos que MAX_AGE_SECONDS (ej. 120s)
void run_cleanup_thread()
{
    const int MAX_AGE_SECONDS = 120; // Mantener 2 minutos de historial en RAM (aprox 60MB)
    
    while(true)
    {
        DIR *dir;
        struct dirent *ent;
        if ((dir = opendir(RAM_PATH)) != NULL)
        {
            time_t now = time(nullptr);
            while ((ent = readdir(dir)) != NULL)
            {
                std::string fname = ent->d_name;
                // Solo procesar archivos segXXXXX.ts
                if (fname.find("seg") == 0 && fname.find(".ts") != std::string::npos) 
                {
                    std::string full_path = std::string(RAM_PATH) + fname;
                    struct stat attr;
                    if (stat(full_path.c_str(), &attr) == 0)
                    {
                        if (difftime(now, attr.st_mtime) > MAX_AGE_SECONDS)
                        {
                            // Es viejo, borrarlo
                            std::remove(full_path.c_str());
                            // std::cout << "[GC] Borrado archivo viejo: " << fname << std::endl;
                        }
                    }
                }
            }
            closedir(dir);
        }
        std::this_thread::sleep_for(std::chrono::seconds(5)); // Revisar cada 5s
    }
}

std::vector<std::string> get_active_segments()
{
    std::vector<std::string> segs;
    std::ifstream list(std::string(RAM_PATH) + "buffer.m3u8");
    std::string line;
    if (list.is_open())
    {
        while (std::getline(list, line))
        {
            if (line.find(".ts") != std::string::npos)
                segs.push_back(std::string(RAM_PATH) + line);
        }
    }
    return segs;
}

std::string get_timestamp_filename()
{
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << OUTPUT_FOLDER << "video_" << std::put_time(std::localtime(&in_time_t), "%Y%m%d_%H%M%S") << ".mp4";
    return ss.str();
}

void replace_all(std::string &str, const std::string &from, const std::string &to)
{
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}

int read_sensor_state(const char *filePath)
{
    std::ifstream file(filePath);
    char state;
    if (file.is_open() && (file >> state))
        return (state == '1') ? 1 : 0;
    return -1;
}

// Copia un archivo de origen a destino de manera eficiente
void copy_file(const std::string &src, const std::string &dst)
{
    std::string cmd = "cp " + src + " " + dst;
    std::system(cmd.c_str());
}

void append_new_segments(std::vector<std::string> &captured, const std::vector<std::string> &current)
{
    if (captured.empty())
    {
        // En modo LINEAL, el orden es trivial: Alfabético = Cronológico
        // seg00100 < seg00101
        captured = current;
        std::sort(captured.begin(), captured.end());
    }
    else
    {
        std::string last = captured.back();
        // Buscar el último capturado en la lista actual
        auto it = std::find(current.begin(), current.end(), last);

        if (it != current.end())
        {
            // Añadir todo lo que esté después
            captured.insert(captured.end(), std::next(it), current.end());
        }
        else
        {
            // Si el último ya no está en la lista activa (raro con buffer de 300s),
            // Buscamos cualquier segmento que sea lexicográficamente mayor
            for(const auto& s : current) {
                if(s > last) captured.push_back(s);
            }
        }
    }
}

int main()
{
    int secondsPostRecord = 8;
    long msPostRecord = secondsPostRecord * 1000;

    ensure_output_folder();
    std::string rtsp_url = get_config_value("RTSP");
    std::string curl_template = get_config_value("CURL");

    if (rtsp_url.empty())
        return 1;

    std::thread t_buffer(run_buffer_thread, rtsp_url);
    t_buffer.detach();

    // Hilo de Limpieza (Garbage Collector)
    std::thread t_cleanup(run_cleanup_thread);
    t_cleanup.detach();

    bool is_recording = false;
    bool waiting_post_record = false;
    std::string current_video_path;
    std::vector<std::string> captured_segments;
    std::chrono::steady_clock::time_point release_time;

    std::cout << "--- Camera Manager v11: Modo Lineal + Garbage Collector ---" << std::endl;

    while (true)
    {
        int sensor = read_sensor_state(SENSOR_27_FILE);

        // --- ESTADO: DETECCIÓN ACTIVA (Sensor en 1) ---
        if (sensor == 1)
        {
            if (!is_recording)
            {
                captured_segments.clear();

                is_recording = true;
                current_video_path = get_timestamp_filename();

                std::cout << "[INFO] Inicio de grabación: " << current_video_path << std::endl;
            }

            if (waiting_post_record)
            {
                // RE-TRIGGER: El sensor detectó algo antes de que pasaran los 3s
                std::cout << "[RE-TRIGGER] Objeto detectado de nuevo. Cancelando cierre." << std::endl;
                waiting_post_record = false;
            }

            // Recolectar fragmentos normalmente
            auto now_segs = get_active_segments();

            // Si es el inicio de la grabación, filtramos para tener solo los últimos 10 segundos de pre-roll
            if (captured_segments.empty())
            {
                std::sort(now_segs.begin(), now_segs.end());
                if (now_segs.size() > 10)
                {
                    // Mantener solo los últimos 10
                    std::vector<std::string> recent_segs(now_segs.end() - 10, now_segs.end());
                    now_segs = recent_segs;
                }
            }

            append_new_segments(captured_segments, now_segs);
        }

        // --- ESTADO: INICIO DE CUENTA ATRÁS (Sensor pasa de 1 a 0) ---
        else if (sensor == 0 && is_recording && !waiting_post_record)
        {
            std::cout << "[POST] Sensor limpio. Esperando 3s para cerrar..." << std::endl;
            release_time = std::chrono::steady_clock::now();
            waiting_post_record = true;
        }

        // --- ESTADO: PROCESANDO ESPERA ---
        if (waiting_post_record)
        {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - release_time).count();

            if (elapsed < msPostRecord)
            {
                // Seguimos capturando mientras esperamos
                auto now_segs = get_active_segments();
                append_new_segments(captured_segments, now_segs);
            }
            else
            {
                // El tiempo expiró sin nuevos triggers: CERRAR VIDEO
                std::cout << "[SISTEMA] Tiempo post-grabación cumplido. Guardando..." << std::endl;

                // Han pasado los 3 segundos, procedemos al cierre normal
                std::cout << "[VIDEO] Finalizando post-grabación..." << std::endl;

                std::ofstream listFile("/dev/shm/concat.txt");
                for (const auto &s : captured_segments)
                    listFile << "file '" << s << "'\n";
                listFile.close();

                std::string concat_cmd = "ffmpeg -y -f concat -safe 0 -i /dev/shm/concat.txt -c copy -an -movflags +faststart " + current_video_path;
                std::system(concat_cmd.c_str());

                if (!curl_template.empty())
                {
                    std::string final_curl = curl_template;
                    replace_all(final_curl, "{FILE_NAME}", current_video_path);
                    std::system((final_curl + " &").c_str());
                }

                // NO BORRAMOS NADA AQUÍ.
                // El Garbage Collector (hilo de limpieza) se encargará de borrar los archivos viejos.
                // Esto permite que si hay otra grabación INMEDIATA, los segmentos sigan ahí disponibles.

                captured_segments.clear();
                is_recording = false;
                waiting_post_record = false;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    return 0;
}