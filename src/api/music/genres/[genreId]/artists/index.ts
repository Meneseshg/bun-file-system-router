const rockArtists = ["Led Zeppelin", "AC/DC", "Deep Purple"];
const popArtists = ["Madonna", "Beyoncé", "Rihanna"];
const jazzArtists = ["Miles Davis", "John Coltrane", "Duke Ellington"];
const classicalArtists = ["Ludwig van Beethoven", "Wolfgang Amadeus Mozart", "Johann Sebastian Bach"];

function getArtistsByGenre(genreId: string) {
  switch (genreId) {
    case "rock":
      return rockArtists;
    case "pop":
      return popArtists;
    case "jazz":
      return jazzArtists;
    case "classical":
      return classicalArtists;
    default:
      return null;
  }
}

export function GET(req: Request, params: { genreId: string }) {
  const { genreId } = params;
  const artists = getArtistsByGenre(genreId);
  if (!artists) {
    return new Response("404 Not Found", { status: 404 });
  }
  return new Response(JSON.stringify(artists), {
    headers: { "Content-Type": "application/json; charset=utf-8" },
  });
}