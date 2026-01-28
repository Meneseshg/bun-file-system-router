export function GET(req: Request) {
  return new Response("GET /api/music/artists/songs/", {
    headers: { "Content-Type": "text/plain; charset=utf-8" },
  });
}