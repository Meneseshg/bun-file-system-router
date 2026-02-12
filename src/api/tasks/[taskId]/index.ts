import { scheduleTask } from "../../../lib/bull";

export async function POST(req: Request, params: { taskId: string }) {
  try {
    const { taskId } = params;
    
    // Parsear body
    let body;
    try {
      body = await req.json();
    } catch (e) {
       return new Response(JSON.stringify({ error: "Invalid JSON body" }), { 
            status: 400,
            headers: { "Content-Type": "application/json" }
        });
    }

    const { time } = body;
    
    // Validaciones
    if (!taskId) {
        return new Response(JSON.stringify({ error: "taskId is required" }), { 
            status: 400,
            headers: { "Content-Type": "application/json" }
        });
    }

    if (typeof time !== 'number' || time < 0) {
        return new Response(JSON.stringify({ error: "Invalid time. Must be a positive number." }), { 
            status: 400,
            headers: { "Content-Type": "application/json" }
        });
    }

    // Programar tarea
    await scheduleTask(taskId, time);

    return new Response(JSON.stringify({ 
        message: `Task ${taskId} scheduled in ${time} seconds`,
        taskId,
        scheduledInSeconds: time
    }), {
        status: 200,
        headers: { "Content-Type": "application/json" }
    });

  } catch (error) {
    console.error("Error scheduling task:", error);
    return new Response(JSON.stringify({ error: "Internal Server Error" }), { 
        status: 500,
        headers: { "Content-Type": "application/json" }
    });
  }
}
