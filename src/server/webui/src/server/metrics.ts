export interface MetricSample {
  ts: number;
  fps: number;
  latencyRms: number;
}

export class RingBuffer<T> {
  private buffer: T[];

  private index = 0;

  private length = 0;

  constructor(private readonly size: number) {
    this.buffer = new Array<T>(size);
  }

  push(value: T): void {
    this.buffer[this.index] = value;
    this.index = (this.index + 1) % this.size;
    if (this.length < this.size) this.length += 1;
  }

  clear(): void {
    this.index = 0;
    this.length = 0;
  }

  toArray(): T[] {
    if (this.length < this.size) return this.buffer.slice(0, this.length);
    return [...this.buffer.slice(this.index), ...this.buffer.slice(0, this.index)];
  }
}

export const metricsBuffer = new RingBuffer<MetricSample>(600);
export const logBuffer = new RingBuffer<string>(2000);

export function getAggregates(samples: MetricSample[]) {
  const count = samples.length || 1; // WHY default to avoid NaN on empty array
  let fpsMin = Number.POSITIVE_INFINITY;
  let fpsMax = Number.NEGATIVE_INFINITY;
  let fpsSum = 0;
  let latencySumSq = 0;
  samples.forEach((s) => {
    fpsMin = Math.min(fpsMin, s.fps);
    fpsMax = Math.max(fpsMax, s.fps);
    fpsSum += s.fps;
    latencySumSq += s.latencyRms ** 2;
  });
  return {
    fpsAvg: fpsSum / count,
    fpsMin: fpsMin === Number.POSITIVE_INFINITY ? 0 : fpsMin,
    fpsMax: fpsMax === Number.NEGATIVE_INFINITY ? 0 : fpsMax,
    latencyRms: Math.sqrt(latencySumSq / count),
  };
}

export function addSample(sample: MetricSample): void {
  metricsBuffer.push(sample);
}

export function addLog(line: string): void {
  logBuffer.push(line);
}
