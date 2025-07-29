import { useQuery } from '@tanstack/react-query';
import { useEffect, useRef, useState } from 'react';

export interface MetricSample {
  ts: number;
  fps: number;
  latencyRms: number;
}

export interface Aggregates {
  fpsAvg: number;
  fpsMin: number;
  fpsMax: number;
  latencyRms: number;
}

const base = '';

export function useStats() {
  return useQuery<Aggregates>({
    queryKey: ['stats'],
    queryFn: () => fetch(`${base}/api/stats`).then((res) => res.json()),
    refetchInterval: 5000,
  });
}

export function useLiveSamples() {
  const [samples, setSamples] = useState<MetricSample[]>([]);
  const wsRef = useRef<WebSocket>();

  useEffect(() => {
    const proto = location.protocol === 'https:' ? 'wss' : 'ws';
    const ws = new WebSocket(`${proto}://${location.host}/stream/stats`);
    wsRef.current = ws;
    let open = true;
    ws.onmessage = (ev) => {
      const sample = JSON.parse(ev.data) as MetricSample;
      setSamples((prev) => {
        const next = [...prev, sample].slice(-600);
        return next;
      });
    };
    ws.onclose = () => {
      open = false;
    };
    return () => {
      if (open) ws.close();
    };
  }, []);
  return samples;
}
