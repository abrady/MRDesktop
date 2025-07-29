import { vi, describe, it, expect } from 'vitest';
import { render, screen } from '@testing-library/react';
import LogTail from '../src/client/components/LogTail';

vi.stubGlobal('fetch', () =>
  Promise.resolve({ text: () => Promise.resolve(new Array(2000).fill('x').join('\n')) }),
);

describe('LogTail', () => {
  it('virtualises long lists', async () => {
    render(<LogTail />);
    const items = await screen.findAllByText('x');
    expect(items.length).toBeLessThan(50);
  });
});
