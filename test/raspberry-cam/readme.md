curl -X POST -F "file=@./grabacion_rtsp.mp4" http://192.168.1.5:3001/api/files/armbian/upload


g++ camera-manager.cpp -o camera-manager -lgpiod
g++ camera-manager.cpp -o camera-manager -pthread

GPIO4 - XSHUT
GPIO2 - SDA
GPIO3 - SCL