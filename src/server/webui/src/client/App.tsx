import { useState } from 'react';
import LogTail from './components/LogTail';
import StatCards from './components/StatCards';
import LiveChart from './components/LiveChart';
import { useLiveSamples } from './api';

export default function App() {
  const [tab, setTab] = useState<'dash' | 'hist'>('dash');
  const samples = useLiveSamples();
  const [help, setHelp] = useState(false);

  useState(() => {
    const onKey = (e: KeyboardEvent) => {
      if (e.key === '?') setHelp((h) => !h);
    };
    window.addEventListener('keydown', onKey);
    return () => window.removeEventListener('keydown', onKey);
  });

  return (
    <div className="app">
      <div className="sidebar">
        <div className="live-indicator" data-on={samples.length > 0} />
        <LogTail />
      </div>
      <div className="main">
        <nav>
          <button type="button" onClick={() => setTab('dash')}>Dashboard</button>
          <button type="button" onClick={() => setTab('hist')}>History</button>
        </nav>
        {tab === 'dash' ? (
          <>
            <StatCards />
            <LiveChart samples={samples} />
          </>
        ) : (
          <LiveChart samples={samples} />
        )}
      </div>
      {help && (
        <div className="help">Press ? to toggle help.</div>
      )}
    </div>
  );
}
