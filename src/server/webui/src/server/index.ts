import express from 'express';
import http from 'http';
import cors from 'cors';
import { routes } from './routes';
import { startWs } from './ws';
import { addSample, addLog } from './metrics';

export function createApp() {
  const app = express();
  if (process.env.NODE_ENV !== 'production') {
    app.use(cors({ origin: 'http://localhost:5173' }));
  }
  app.use(routes);
  if (process.env.NODE_ENV === 'production') {
    // WHY serve built client when in production
    app.use(express.static(new URL('../../dist/client', import.meta.url).pathname));
  }
  return app;
}
export function createServer() {
  const app = createApp();
  const server = http.createServer(app);
  startWs(server);
  return server;
}

const server = createServer();

// emit fake samples to have data without real server
setInterval(() => {
  const sample = {
    ts: Date.now(),
    fps: 60 + Math.random() * 5,
    latencyRms: 5 + Math.random(),
  };
  addSample(sample);
  addLog(`sample: ${JSON.stringify(sample)}`);
}, 1000);

if (process.env.NODE_ENV !== 'test') {
  const PORT = Number(process.env.PORT) || 3000;
  server.listen(PORT, () => {
    console.log(`server listening on ${PORT}`);
  });
}
