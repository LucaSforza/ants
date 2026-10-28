"""Reproducible finite-horizon diagnostics, not a chaos/periodicity detector."""
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
binary = ROOT / "build/ant-headless"
results = []
for preset in ("orbit", "balanced", "broad", "fast-turn"):
    for dt in (.01, .005):
        args = [str(binary), "--preset", preset, "--dt", str(dt),
                "--steps", str(round(500 / dt))]
        result = json.loads(subprocess.check_output(args, text=True))
        results.append({"preset": preset, "dt": dt, "resolution": 512, **result})
        print(f"{preset:10s} dt={dt:g} span=({result['span_x']:.3f},"
              f"{result['span_y']:.3f}) displacement={result['displacement']:.3f} "
              f"turn changes={result['turn_sign_changes']} finite={result['finite']}")
assert all(row["finite"] for row in results)
output = ROOT / "experiments/results.json"
output.write_text(json.dumps({
    "description": "All-black field; center start, theta=pi/2; L=128; T=500. "
                   "Spans/displacement use unwrapped trajectory. Sign changes sample c>0.5 "
                   "at step endpoints. Finite-horizon diagnostics only.",
    "runs": results,
}, indent=2) + "\n")
print(f"Wrote {output.relative_to(ROOT)}")
