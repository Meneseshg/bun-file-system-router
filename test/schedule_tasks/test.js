import http from 'k6/http';
import { check, sleep } from 'k6';

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
    { duration: '30s', target: 20 },  // Rampa de subida
    { duration: '1m', target: 50 },   // Mantener 50 usuarios concurrentes
    { duration: '30s', target: 0 },   // Rampa de bajada
  ],
};

export default function () {
  const randomShortId = generateRandomShortId(8);
  const url = `http://localhost:3001/api/tasks/${randomShortId}`;

  // Definimos el cuerpo de la petición
  const payload = JSON.stringify({
    time: 5,
  });

  // Definimos los encabezados (importante para que la API sepa que es JSON)
  const params = {
    headers: {
      'Content-Type': 'application/json',
    },
  };

  const res = http.post(url, payload, params);

  // Verificaciones de éxito
  const success = check(res, {
    'status es 200 o 201': (r) => r.status === 200 || r.status === 201,
    'respuesta válida JSON': (r) => r.json() !== null,
  });

  // Log de errores si la inserción falla
  if (!success) {
    console.error(`❌ Error en Task ${randomShortId} | Status: ${res.status} | Body: ${res.body}`);
  }

  // Ajustamos el sleep a 0.2 para mantener la consistencia con tus pruebas anteriores
  sleep(0.2);
}