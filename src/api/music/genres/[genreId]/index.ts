export function GET(req: Request) {
 return new Response("GET /api/music/genres/[genreId]/", {
    headers: { "Content-Type": "application/json; charset=utf-8" },
  });
}