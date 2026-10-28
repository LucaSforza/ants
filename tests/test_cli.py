"""Headless CLI regression tests; Python standard library only."""
import csv
import json
import os
from pathlib import Path
import subprocess
import tempfile

BIN = str(Path(os.environ.get("ANT_BINARY", "build/ant-headless")).resolve())


def run(*args, ok=True):
    result = subprocess.run([BIN, *args], text=True, capture_output=True)
    assert (result.returncode == 0) == ok, (args, result.stdout, result.stderr)
    return result


for args in [
    ["--dt", "nan"], ["--speed", "inf"], ["--dt", "0"],
    ["--resolution", "1e100"], ["--resolution", "256.5"],
    ["--sigma", "0"], ["--omega", "-1"], ["--steps", "-2"],
    ["--steps", "0"], ["--steps", "18446744073709551616"],
    ["--every", "0"], ["--preset", "unknown"], ["--rate", "0"],
    ["--unknown"], ["--dt"], ["extra"], ["--resolution", "64"],
]:
    run(*args, ok=False)

assert "midpoint" in run("--help").stdout
with tempfile.TemporaryDirectory(prefix="ant-test-", dir="build") as directory:
    path = str(Path(directory) / "trajectory.csv")
    a = json.loads(run("--steps", "1000", "--csv", path, "--every", "300").stdout)
    b = json.loads(run("--steps", "1000", "--rate", "128", "--trail").stdout)
    assert a == b  # output sampling/render rate cannot influence the model
    assert a["finite"] and a["time"] == 10 and a["steps"] == 1000
    with open(path) as f:
        rows = list(csv.DictReader(f))
    assert [int(row["steps"]) for row in rows] == [0, 300, 600, 900, 1000]
    assert float(rows[-1]["x"]) == a["x"]
    run("--steps", "1", "--csv", directory, ok=False)

a = run("--preset", "orbit", "--kappa", "2", "--steps", "100").stdout
b = run("--kappa", "2", "--preset", "orbit", "--steps", "100").stdout
assert a == b
print("CLI validation, deterministic sampling, overrides, and CSV tests passed.")
