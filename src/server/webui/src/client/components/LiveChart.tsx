import { LinePath } from "@visx/shape";
import { scaleLinear } from "@visx/scale";

import type { MetricSample } from "../api";
interface Props {
  samples: MetricSample[];
}

export default function LiveChart({ samples }: Props) {
  if (samples.length === 0) return null;
  const width = 400;
  const height = 200;
  const x = scaleLinear({ domain: [samples[0].ts, samples[samples.length - 1].ts], range: [0, width] });
  const fps = scaleLinear({ domain: [0, 120], range: [height, 0] });
  const lat = scaleLinear({ domain: [0, 20], range: [height, 0] });
  return (
    <svg width={width} height={height} style={{ background: '#111' }}>
      <LinePath
        data={samples}
        x={(d) => x(d.ts)}
        y={(d) => fps(d.fps)}
        stroke="lime"
      />
      <LinePath
        data={samples}
        x={(d) => x(d.ts)}
        y={(d) => lat(d.latencyRms)}
        stroke="cyan"
      />
    </svg>
  );
}
