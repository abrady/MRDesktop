import { Router } from 'express';
import { metricsBuffer, logBuffer, getAggregates } from './metrics';

export const routes = Router();

routes.get('/api/stats', (_req, res) => {
  const samples = metricsBuffer.toArray().slice(-60);
  res.json(getAggregates(samples));
});

routes.get('/api/stats/history', (_req, res) => {
  res.json(metricsBuffer.toArray());
});

routes.get('/api/log', (_req, res) => {
  res.type('text/plain').send(logBuffer.toArray().join('\n'));
});
