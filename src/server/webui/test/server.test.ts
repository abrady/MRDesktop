import { describe, it, expect, beforeEach } from 'vitest';
import request from 'supertest';
import { RingBuffer, addSample, metricsBuffer } from '../src/server/metrics';
import { createApp } from '../src/server/index';

describe('RingBuffer', () => {
  it('overwrites old items when full', () => {
    const buf = new RingBuffer<number>(3);
    buf.push(1);
    buf.push(2);
    buf.push(3);
    buf.push(4);
    expect(buf.toArray()).toEqual([2, 3, 4]);
  });
});

describe('GET /api/stats', () => {
  beforeEach(() => {
    metricsBuffer.clear();
    addSample({ ts: 0, fps: 1, latencyRms: 1 });
  });
  it('returns aggregates', async () => {
    const app = createApp();
    const res = await request(app).get('/api/stats');
    expect(res.status).toBe(200);
    expect(res.body).toHaveProperty('fpsAvg');
  });
});
