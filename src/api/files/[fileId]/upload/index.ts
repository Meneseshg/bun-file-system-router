import { mkdir } from "node:fs/promises";
import { join } from "node:path";

export async function POST(req: Request, params: { fileId: string }) {
  const { fileId } = params;

  if (!fileId) {
    return new Response("Missing fileId parameter", { status: 400 });
  }

  const MAX_SIZE = 100 * 1024 * 1024; // 20MB

  // 1. Verificar tamaño por Content-Length (si existe) para rechazo rápido
  const contentLength = req.headers.get("content-length");
  if (contentLength && parseInt(contentLength) > MAX_SIZE) {
    return new Response("File too large (max 20MB)", { status: 413 });
  }

  try {
    // Definir directorio de destino: ./storage/[fileId]/
    // Usamos process.cwd() para asegurar que sea relativo a la raíz del proyecto
    const storageDir = join(process.cwd(), "storage", fileId);

    // Crear directorio si no existe (recursivo)
    await mkdir(storageDir, { recursive: true });

    const contentType = req.headers.get("content-type") || "";

    // A. Manejo de Multipart Form Data (Estándar para subida de archivos)
    if (contentType.includes("multipart/form-data")) {
      const formData = await req.formData();
      const filesSaved: string[] = [];

      for (const [key, value] of formData.entries()) {
        if (value instanceof File) {
          const fileName = value.name || `unknown_file_${Date.now()}.mp4`;
          if (value.size > MAX_SIZE) {
            return new Response(`File ${fileName} too large (max 20MB)`, {
              status: 413,
            });
          }

          const filePath = join(storageDir, fileName);
          await Bun.write(filePath, value);
          filesSaved.push(fileName);
        }
      }

      if (filesSaved.length === 0) {
        return new Response("No file found in form data", { status: 400 });
      }

      console.log("Files saved");

      return new Response(
        JSON.stringify({
          message: "Files uploaded successfully",
          files: filesSaved,
          location: storageDir,
        }),
        {
          headers: { "Content-Type": "application/json" },
        },
      );
    }

    // B. Manejo de Raw Body (Binario directo)
    // Si envían el archivo directamente en el body sin form-data
    else {
      // Leemos el body como ArrayBuffer
      const arrayBuffer = await req.arrayBuffer();

      if (arrayBuffer.byteLength > MAX_SIZE) {
        return new Response("File too large (max 20MB)", { status: 413 });
      }

      if (arrayBuffer.byteLength === 0) {
        return new Response("Empty body", { status: 400 });
      }

      // Generamos un nombre genérico ya que raw body no tiene metadatos de nombre
      // Intentamos deducir extensión del content-type si es posible, o usar .bin
      const mimeType =
        contentType.split(";")[0]?.trim() || "application/octet-stream";
      const extension = mimeType.split("/")[1] || "bin";
      const fileName = `upload_${Date.now()}.${extension}`;
      const filePath = join(storageDir, fileName);

      await Bun.write(filePath, arrayBuffer);

      return new Response(
        JSON.stringify({
          message: "File uploaded successfully (raw)",
          file: fileName,
          location: storageDir,
        }),
        {
          headers: { "Content-Type": "application/json" },
        },
      );
    }
  } catch (error) {
    console.error("Upload error:", error);
    return new Response("Internal Server Error during upload", { status: 500 });
  }
}
