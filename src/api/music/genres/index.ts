export function GET(req: Request) {
  return new Response("GET /api/music/genres/", {
    headers: { "Content-Type": "text/plain; charset=utf-8" },
  });
}