const rockSongs = ["Stairway to Heaven", "Hotel California", "Imagine"];
const popSongs = ["Shape of You", "Blinding Lights", "Someone Like You"];
const jazzSongs = ["Smooth", "Blue in Green", "Take Five"];
const classicalSongs = ["Concerto for Strings and Piano", "Eine Kleine Nachtmusik", "Turkish Symphony No. 5"];

function getSongsByGenre(genreId: string) {
  switch (genreId) {
    case "rock":
      return rockSongs;
    case "pop":
      return popSongs;
    case "jazz":
      return jazzSongs;
    case "classical":
      return classicalSongs;
    default:
      return null;
  }
}

export function GET(req: Request, params: { genreId: string }) {
  const { genreId } = params;
  const songs = getSongsByGenre(genreId);
  if (!songs) {
    return new Response("404 Not Found", { status: 404 });
  }
  return new Response(JSON.stringify(songs), {
    headers: { "Content-Type": "application/json; charset=utf-8" },
  });
}