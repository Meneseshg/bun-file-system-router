export function GET(req: Request) {
  return new Response("GET /api/users/", {
    headers: { "Content-Type": "text/plain; charset=utf-8" },
  });
}

export async function POST(req: Request) {
  try {
    const body = await req.json();
    return new Response(`POST /api/users/ ${JSON.stringify(body)}`, {
      headers: { "Content-Type": "text/plain; charset=utf-8" },
    });
  } catch (error) {
    return new Response("Error parsing JSON", {
      status: 400,
      headers: { "Content-Type": "text/plain; charset=utf-8" },
    });
  }
}

export async function PUT(req: Request) {
  try {
    const body = await req.json();
    return new Response(`PUT /api/users/ ${JSON.stringify(body)}`, {
      headers: { "Content-Type": "text/plain; charset=utf-8" },
    });
  } catch (error) {
    return new Response("Error parsing JSON", {
      status: 400,
      headers: { "Content-Type": "text/plain; charset=utf-8" },
    });
  }
}

export function DELETE(req: Request) {
  return new Response("DELETE /api/users/", {
    headers: { "Content-Type": "text/plain; charset=utf-8" },
  });
}
