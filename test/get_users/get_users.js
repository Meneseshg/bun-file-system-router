import http from 'k6/http';
import { check, sleep } from 'k6';

export const options = {
  stages: [
    { duration: '10s', target: 5 },  // Rampa de subida a 20 usuarios
    { duration: '10s', target: 10 },  // Rampa de subida a 20 usuarios
    { duration: '30s', target: 50 },   // Stress test con 50 usuarios (como en tus pruebas anteriores)
    { duration: '10s', target: 10 },   // Rampa de bajada
  ],
};

export default function () {
  const url = 'http://localhost:3000/api/users';

  // "discardResponse: true" es útil si solo quieres medir el rendimiento sin procesar el body.
  // Sin embargo, para validar contenido, lo dejamos por defecto pero limitamos el log.
  const res = http.get(url);

  const success = check(res, {
    'status es 200': (r) => r.status === 200,
    // Verificamos que el cuerpo no esté vacío sin imprimirlo todo
    'cuerpo contiene datos': (r) => r.body && r.body.length > 0,
  });

  if (!success) {
    // Solo imprimimos los primeros 100 caracteres para evitar saturar la terminal
    const errorPreview = res.body ? res.body.substring(0, 100) : 'Sin respuesta';
    console.error(`❌ Error | Status: ${res.status} | Preview: ${errorPreview}...`);
  }

  // Aumentamos ligeramente el sleep para permitir que el GC (Garbage Collector) 
  // de k6 limpie la respuesta anterior de la memoria.
//   sleep(0.2);
}