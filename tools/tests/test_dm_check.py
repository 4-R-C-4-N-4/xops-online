"""The DM map gate itself must work: jump calibration matches the guide and
the rebuilt sealed hangar passes using real engine collision and physics."""
import pathlib
import re
import shutil
import subprocess
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]


def run(*args):
    r = subprocess.run([sys.executable, str(ROOT / "tools/dmcheck.py"), *args], cwd=ROOT,
                       capture_output=True, text=True, timeout=900)
    return r.returncode, r.stdout


def metric(out, name):
    m = re.search(rf"^METRIC {re.escape(name)} ([-0-9.]+)", out, re.M)
    return float(m.group(1)) if m else None


@unittest.skipUnless(shutil.which("g++"), "g++ required for the real-engine map gate")
class DmCheckTests(unittest.TestCase):
    def test_rebuilt_hangar_passes_sealed_engine_gate(self):
        code, out = run("data/dmhangar", "hangar.pd1", "--sealed")
        self.assertEqual(code, 0, out)
        self.assertIn("DMCHECK OK", out)
        self.assertEqual(metric(out, "lifts"), 2, out)

    def test_jump_limit_is_16(self):
        code, out = run("--jumpprobe")
        self.assertEqual(code, 0, out)
        self.assertEqual(metric(out, "jump_up_max"), 16.0, out)
        self.assertEqual(metric(out, "jump_step_17"), 0.0, out)


if __name__ == "__main__":
    unittest.main()
