/**
 * Implementación de API con File System Router nativo de Bun.
 * Este archivo actúa como el punto de entrada y orquestador del servidor.
 */

// 1. Inicializamos el enrutador de sistema de archivos.
// Definimos la ruta absoluta al directorio "src" para que las rutas coincidan con la estructura de carpetas.
// Al apuntar a "src", la carpeta "api" dentro de ella formará parte de la ruta (ej: /api/users).
const ROUTES_DIR = import.meta.dir + "/src";

const router = new Bun.FileSystemRouter({
  style: "nextjs",
  dir: ROUTES_DIR,
});

// Nota: router.dir puede no estar disponible en todas las versiones de los tipos, 
// así que usamos nuestra variable ROUTES_DIR para el log.
console.log(`🚀 Router inicializado. Escaneando rutas en: ${ROUTES_DIR}`);

// 2. Iniciamos el servidor HTTP con Bun.serve
const server = Bun.serve({
  port: 3000,
  
  // La función fetch maneja todas las peticiones entrantes
  async fetch(req) {
    // 3. Buscamos si la URL solicitada coincide con algún archivo en las rutas
    // Al usar "src" como directorio base, y tener "api" dentro, 
    // las URLs deben incluir /api (ej: http://localhost:3000/api/users)
    const match = router.match(req);
      
    // Si encontramos una coincidencia (match no es null)
    if (match) {
      try {
        // 4. Importamos dinámicamente el archivo correspondiente a la ruta.
        // match.filePath contiene la ruta absoluta al archivo en disco.
        const component = await import(match.filePath);
        
        // 5. Lógica de Despacho (Dispatch Logic)
        const method = req.method; // ej: "GET", "POST"

        // a) Buscamos una exportación nombrada que coincida con el método HTTP (estilo App Router)
        //    ej: export function GET(req) { ... }
        if (component[method] && typeof component[method] === "function") {
          return component[method](req, match.params);
        }

        // b) Fallback: Si no hay exportación por método, buscamos 'export default' (estilo tradicional)
        if (component.default && typeof component.default === "function") {
          return component.default(req, match.params);
        }

        // c) Si no hay ni método específico ni default, devolvemos 405 Method Not Allowed
        //    (Opcionalmente, podríamos verificar si hay otras exportaciones para llenar el header 'Allow')
        return new Response(`Method ${method} Not Allowed`, { status: 405 });

      } catch (error) {
        console.error("Error al ejecutar el endpoint:", error);
        return new Response("Error interno del servidor", { status: 500 });
      }
    }

    // 6. Si no hay coincidencia, devolvemos un 404
    return new Response("404 Not Found", { status: 404 });
  },
});

console.log(`Servidor escuchando en http://localhost:${server.port}`);
