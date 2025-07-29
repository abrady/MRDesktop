#!/usr/bin/env python3
"""
Capture a 10-second Perfetto trace from an Android / Quest device.
Usage:
    python perfetto_capture.py            # saves mrdesk.pb in the cwd
"""
from __future__ import annotations
import subprocess
import sys
from pathlib import Path
import textwrap

# ── CONFIGURATION ────────────────────────────────────────────────────────────
PACKAGE_NAME  = "com.mrdesktop"
TRACE_ON_DEV  = "/data/misc/perfetto-traces/mrdesk.pb"
TRACE_LOCAL   = Path("mrdesk.pb")
DURATION_MS   = 10_000                 # 10 s
USE_PID       = True                   # False → use target_cmdline instead
# ──────────────────────────────────────────────────────────────────────────────

def adb(*args, **kw):
    """Run an adb command and propagate errors nicely."""
    return subprocess.run(["adb", *args], check=True, **kw)

def get_pid(pkg: str) -> str:
    """Return the first PID for the given package (raises if not running)."""
    out = subprocess.check_output(
        ["adb", "shell", "pidof", "-s", pkg], text=True,
        stderr=subprocess.DEVNULL
    ).strip()
    if not out:
        sys.exit(f"❌  Package {pkg!r} isn’t running.")
    return out

def build_config(pid: str | None = None) -> str:
    """Return a text-proto Perfetto TraceConfig."""
    if pid:
        perf_scope = f"target_pid: {pid}"
    else:
        # Android 12+ supports target_cmdline
        perf_scope = (
            'callstack_sampling {'
            f' scope {{ target_cmdline: "{PACKAGE_NAME}" }}'
            ' kernel_frames: false }'
        )

    return textwrap.dedent(f"""
        buffers: {{ size_kb: 32768 }}
        duration_ms: {DURATION_MS}
        flush_period_ms: 5000

        # ── Kernel scheduler + atrace ─────────────────────────────────────────
        data_sources {{
          config {{
            name: "linux.ftrace"
            ftrace_config {{
              atrace_categories: "gfx,view,wm,video,input,net,sched"
              atrace_apps: "{PACKAGE_NAME}"
              ftrace_events: "sched/sched_switch"
              ftrace_events: "sched/sched_wakeup"
            }}
          }}
        }}

        # ── Native CPU Flamegraph ─────────────────────────────────────────────
        data_sources {{
          config {{
            name: "linux.perf"
            perf_event_config {{
              sampling_frequency: 1000
              {perf_scope}
              callstack_sampling {{ kernel_frames: false }}
            }}
          }}
        }}

        # ── GPU + FrameTimeline ───────────────────────────────────────────────
        data_sources {{ config {{ name: "android.gpu.renderstages" }} }}
        data_sources {{ config {{ name: "android.surfaceflinger.frametimeline" }} }}

        # ── Process / thread metadata ─────────────────────────────────────────
        data_sources {{
          config {{
            name: "linux.process_stats"
            process_stats_config {{
              scan_all_processes_on_start: true
              record_thread_names: true
            }}
          }}
        }}
    """).lstrip()

def main() -> None:
    pid = None
    if USE_PID:
        print(f"🔍  Looking up PID for {PACKAGE_NAME}")
        pid = get_pid(PACKAGE_NAME)
        print("    → PID", pid)

    print(f"▶️  Starting Perfetto trace ({DURATION_MS} ms)…")
    cfg = build_config(pid)
    adb("shell", "perfetto", "--txt", "-o", TRACE_ON_DEV, "-c", "-", input=cfg.encode())

    print("⬇️  Pulling trace to", TRACE_LOCAL)
    adb("pull", TRACE_ON_DEV, str(TRACE_LOCAL))
    print("✅  Done! Trace saved as", TRACE_LOCAL)

if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as exc:
        sys.exit(f"❌  adb command failed: {exc}")
