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
#include <sys/stat.h> // Para crear carpetas

// --- Configuración de Rutas ---
const char* CONFIG_FILE = "camera.url";
const char* SENSOR_27_FILE = "/dev/shm/pin_27_state";
const char* RAM_PATH = "/dev/shm/";
const std::string OUTPUT_FOLDER = "grabaciones/";

// Función para asegurar que la carpeta de destino exista
void ensure_output_folder() {
    struct stat info;
    if (stat(OUTPUT_FOLDER.c_str(), &info) != 0) {
        std::cout << "[SISTEMA] Creando carpeta: " << OUTPUT_FOLDER << std::endl;
        std::system(("mkdir -p " + OUTPUT_FOLDER).c_str());
    }
}

// Función para extraer valores del archivo de configuración
std::string get_config_value(const std::string& key) {
    std::ifstream file(CONFIG_FILE);
    std::string line;
    if (file.is_open()) {
        while (std::getline(file, line)) {
            if (line.find(key + "=") == 0) return line.substr(key.length() + 1);
        }
    }
    return "";
}

// Hilo del Buffer Persistente
void run_buffer_thread(std::string rtsp_url) {
    // Mantiene los fragmentos en RAM de forma circular
    std::string buffer_cmd = "ffmpeg -hide_banner -loglevel error -rtsp_transport tcp -i \"" + rtsp_url + 
                             "\" -c copy -an -f segment -segment_time 1 -segment_list " + RAM_PATH + "buffer.m3u8 " +
                             "-segment_list_size 3 -segment_wrap 15 -flags +global_header " +
                             RAM_PATH + "seg%03d.ts";
    
    while(true) {
        std::cout << "[BUFFER] Conectando a cámara..." << std::endl;
        std::system(buffer_cmd.c_str());
        std::this_thread::sleep_for(std::chrono::seconds(5)); // Reintento en caso de caída
    }
}

std::vector<std::string> get_active_segments() {
    std::vector<std::string> segs;
    std::ifstream list(std::string(RAM_PATH) + "buffer.m3u8");
    std::string line;
    if (list.is_open()) {
        while (std::getline(list, line)) {
            if (line.find(".ts") != std::string::npos) segs.push_back(std::string(RAM_PATH) + line);
        }
    }
    return segs;
}

std::string get_timestamp_filename() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << OUTPUT_FOLDER << "video_" << std::put_time(std::localtime(&in_time_t), "%Y%m%d_%H%M%S") << ".mp4";
    return ss.str();
}

void replace_all(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}

int read_sensor_state(const char* filePath) {
    std::ifstream file(filePath);
    char state;
    if (file.is_open() && (file >> state)) return (state == '1') ? 1 : 0;
    return -1;
}

int main() {
    ensure_output_folder();
    std::string rtsp_url = get_config_value("RTSP");
    std::string curl_template = get_config_value("CURL");

    if (rtsp_url.empty()) return 1;

    std::thread t_buffer(run_buffer_thread, rtsp_url);
    t_buffer.detach();

    bool is_recording = false;
    bool waiting_post_record = false;
    std::string current_video_path;
    std::set<std::string> captured_segments;
    
    // Timer para la post-grabación
    std::chrono::steady_clock::time_point release_time;

    std::cout << "--- Camera Manager v9: Pre (10s) y Post (3s) Grabación ---" << std::endl;

    while (true) {
        int sensor = read_sensor_state(SENSOR_27_FILE);

        // CASO 1: Botón presionado
        if (sensor == 1) {
            if (!is_recording) {
                is_recording = true;
                current_video_path = get_timestamp_filename();
                std::cout << "[INFO] Grabando: " << current_video_path << std::endl;
            }
            waiting_post_record = false; // Resetear si se vuelve a pulsar
            
            auto now_segs = get_active_segments();
            for (const auto& s : now_segs) captured_segments.insert(s);
        } 
        // CASO 2: Botón soltado (Iniciar cuenta atrás de 3 segundos)
        else if (sensor == 0 && is_recording && !waiting_post_record) {
            std::cout << "[POST] Botón soltado. Grabando 3s extra..." << std::endl;
            release_time = std::chrono::steady_clock::now();
            waiting_post_record = true;
        }

        // CASO 3: Durante la espera de los 3 segundos adicionales
        if (waiting_post_record) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - release_time).count();

            if (elapsed < 3) {
                // Seguimos capturando segmentos durante estos 3 segundos
                auto now_segs = get_active_segments();
                for (const auto& s : now_segs) captured_segments.insert(s);
            } else {
                // Han pasado los 3 segundos, procedemos al cierre normal
                std::cout << "[VIDEO] Finalizando post-grabación..." << std::endl;

                std::ofstream listFile("/dev/shm/concat.txt");
                for (const auto& s : captured_segments) listFile << "file '" << s << "'\n";
                listFile.close();

                std::string concat_cmd = "ffmpeg -y -f concat -safe 0 -i /dev/shm/concat.txt -c copy -an -movflags +faststart " + current_video_path;
                std::system(concat_cmd.c_str());

                if (!curl_template.empty()) {
                    std::string final_curl = curl_template;
                    replace_all(final_curl, "{FILE_NAME}", current_video_path);
                    std::system((final_curl + " &").c_str());
                }

                captured_segments.clear();
                is_recording = false;
                waiting_post_record = false;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    return 0;
}