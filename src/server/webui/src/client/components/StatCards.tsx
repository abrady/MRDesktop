import { useStats } from '../api';

export default function StatCards() {
  const { data } = useStats();
  if (!data) return null;
  return (
    <div style={{ display: 'flex', gap: '1rem' }}>
      <div>avg fps: {data.fpsAvg.toFixed(1)}</div>
      <div>min fps: {data.fpsMin.toFixed(1)}</div>
      <div>max fps: {data.fpsMax.toFixed(1)}</div>
      <div>lat rms: {data.latencyRms.toFixed(2)}ms</div>
    </div>
  );
}
