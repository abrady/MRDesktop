import { WebSocketServer, WebSocket } from 'ws';
import type { Server as HttpServer } from 'http';
import { metricsBuffer } from './metrics';

interface HeartbeatWs extends WebSocket {
  isAlive?: boolean;
}

export function startWs(server: HttpServer): void {
  const wss = new WebSocketServer({ server, path: '/stream/stats' });

  wss.on('connection', (ws: HeartbeatWs) => {
    ws.isAlive = true;
    ws.on('pong', () => {
      ws.isAlive = true;
    });
  });

  const broadcast = setInterval(() => {
    const latest = metricsBuffer.toArray().slice(-1)[0];
    if (!latest) return;
    const data = JSON.stringify(latest);
    wss.clients.forEach((client) => {
      if (client.readyState === WebSocket.OPEN) client.send(data);
    });
  }, 1000);

  const heartbeat = setInterval(() => {
    wss.clients.forEach((client) => {
      const c = client as HeartbeatWs;
      if (!c.isAlive) {
        c.terminate();
        return;
      }
      c.isAlive = false;
      c.ping();
    });
  }, 30000);

  wss.on('close', () => {
    clearInterval(broadcast);
    clearInterval(heartbeat);
  });
}
