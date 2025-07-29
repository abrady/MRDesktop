import { useEffect, useRef, useState } from 'react';

export default function LogTail() {
  const [lines, setLines] = useState<string[]>([]);
  const ref = useRef<HTMLDivElement>(null);

  useEffect(() => {
    const load = () => fetch("/api/log").then((r) => r.text()).then((t) => setLines(t.split("\n")));
    load();
    const id = setInterval(load, 5000);
    return () => clearInterval(id);
  }, []);

  useEffect(() => {
    if (ref.current?.scrollTo) ref.current.scrollTo(0, ref.current.scrollHeight);
  }, [lines]);

  const rowHeight = 18;
  const [scroll, setScroll] = useState(0);
  const visible = 20; // show 20 rows at a time
  const total = lines.length;
  const start = Math.floor(scroll / rowHeight);
  const end = Math.min(start + visible, total);
  const offsetY = start * rowHeight;

  return (
    <div
      ref={ref}
      onScroll={(e) => setScroll((e.target as HTMLDivElement).scrollTop)}
      style={{ overflowY: 'auto', height: visible * rowHeight }}
    >
      <div style={{ height: total * rowHeight, position: 'relative' }}>
        <div style={{ position: 'absolute', top: offsetY, left: 0, right: 0 }}>
          {lines.slice(start, end).map((l, i) => (
            <div key={start + i} style={{ fontFamily: 'monospace', height: rowHeight }}>
              {l}
            </div>
          ))}
        </div>
      </div>
    </div>
  );
}
