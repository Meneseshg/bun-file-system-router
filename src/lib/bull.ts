import Queue from 'bull';
import { prisma } from './db';

// Crear la cola
export const taskQueue = new Queue('simple-tasks', {
  redis: {
    port: 6379,
    host: 'localhost',
  },
});

// Definir el procesador
taskQueue.process(async (job) => {
  const { taskId } = job.data;
  console.debug(`[Worker] Iniciando tarea: ${taskId}`);
  
  try {
    const count = await prisma.usuario.count();
    console.debug(`[Worker] Tarea ${taskId}: Hay ${count} usuarios en la base de datos.`);
  } catch (error) {
    console.error(`[Worker] Error en tarea ${taskId}:`, error);
    throw error;
  }
});

/**
 * Programa una tarea para ejecutarse después de cierto tiempo.
 * @param taskId Identificador de la tarea
 * @param delaySeconds Tiempo de espera en segundos
 */
export const scheduleTask = async (taskId: string, delaySeconds: number) => {
  // Bull usa milisegundos para delay
  const delay = delaySeconds * 1000;
  
  await taskQueue.add(
    { taskId },
    {
      jobId: taskId, // Usar taskId como ID del trabajo
      delay: delay,
      removeOnComplete: true, // Opcional: limpiar al terminar
    }
  );
  
  console.debug(`[Queue] Tarea ${taskId} programada para ejecutarse en ${delaySeconds} segundos.`);
};
