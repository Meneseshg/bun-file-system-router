import { prisma } from "../../lib/db";

export async function GET(req: Request) {
  try {
    const usuarios = await prisma.usuario.findMany();
    console.log("Fetched users");
    return new Response(JSON.stringify(usuarios), {
      headers: { "Content-Type": "application/json; charset=utf-8" },
    });
  } catch (error) {
    console.error("Error fetching users:", error);
    return new Response(JSON.stringify({ error: "Internal Server Error" }), {
      status: 500,
      headers: { "Content-Type": "application/json; charset=utf-8" },
    });
  }
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
