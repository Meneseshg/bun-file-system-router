import http from 'k6/http';
import { check, sleep } from 'k6';

// 1. Cargamos el archivo de texto en memoria
// Asegúrate de que "note.txt" esté en la misma carpeta que este script
const fileData = open('./note.txt');

// Función para generar el randomShortId (alfanumérico de 8 dígitos)
function generateRandomShortId(length) {
  const characters = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
  let result = '';
  for (let i = 0; i < length; i++) {
    result += characters.charAt(Math.floor(Math.random() * characters.length));
  }
  return result;
}

export const options = {
  stages: [
    { duration: '10s', target: 5 },  // Rampa de subida a 20 usuarios
    { duration: '10s', target: 10 },  // Rampa de subida a 20 usuarios
    { duration: '30s', target: 30 },   // Stress test con 50 usuarios (como en tus pruebas anteriores)
    { duration: '10s', target: 10 },   // Rampa de bajada
  ],
};

export default function () {
  const randomShortId = generateRandomShortId(8);
  const url = `http://localhost:3001/api/files/${randomShortId}/upload`;

  const data = {
    // El nombre del campo 'file' debe coincidir con lo que espera tu API
    file: http.file(fileData, 'note.txt', 'text/plain'),
  };

  const res = http.post(url, data);

  // Verificaciones basadas en tus estándares de éxito previos
  const success = check(res, {
    'status es 200 o 201': (r) => r.status === 200 || r.status === 201,
    'tiempo de respuesta < 500ms': (r) => r.timings.duration < 500,
  });

  // Registro de errores detallado
  if (!success) {
    console.error(`❌ Fallo en ID: ${randomShortId} | Status: ${res.status} | Body: ${res.body}`);
  }

  sleep(0.1); // Pausa breve para mantener un flujo constante de peticiones
}