/**
 * Endpoint raíz: GET /
 * Usa exportación nombrada GET.
 */
export function GET(req: Request) {
  return new Response("¡Hola desde el File System Router de Bun (Estilo App Router v2)! 🥟", {
    headers: { "Content-Type": "text/plain; charset=utf-8" },
  });
}
