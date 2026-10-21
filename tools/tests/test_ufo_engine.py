"""Compile and execute real engine wave/parameter logic with device stubs."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class UfoEngineTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++"), "g++ required for native engine proof")
    def test_actual_engine_waves_and_completion(self):
        sources = [ROOT / "tests/ufo/engine_harness.cpp"]
        names = ("ai collision config d3dgraphics-debugfont datafile debug event gamemain "
                 "gui-object object objectmanager parameter resource scene soundmanager statemachine").split()
        sources += [ROOT / "src" / (name + ".cpp") for name in names]
        sources += sorted((ROOT / "src/net").glob("*.cpp"))  # XOPS ONLINE session/protocol
        sources += [ROOT / "src/platform/common.cpp", ROOT / "src/platform/input-common.cpp",
                    ROOT / "src/platform/vfs.cpp"]
        sources += [ROOT / "src/platform/stub" / (name + "-stub.cpp")
                    for name in ("window", "input", "sound", "d3dgraphics")]
        with tempfile.TemporaryDirectory(prefix="ufo-engine-", dir=os.environ.get("TMPDIR")) as temp:
            binary = Path(temp) / "engine"
            command = ["g++", "-std=c++17", "-O0", "-g", "-DOPENXOPS_PLATFORM_STUB",
                       "-fsanitize=undefined", "-fno-sanitize-recover=undefined",
                       "-I", str(ROOT / "src"), *map(str, sources), "-lz", "-o", str(binary)]
            build = subprocess.run(command, capture_output=True, text=True, timeout=180)
            self.assertEqual(build.returncode, 0, build.stdout + build.stderr)
            run = subprocess.run([str(binary), str(ROOT / "assets/pack")],
                                 capture_output=True, text=True, timeout=30)
            self.assertEqual(run.returncode, 0, run.stdout + run.stderr)
            self.assertIn("EXPOSED PLAYER SURVIVAL OK", run.stdout)
            self.assertIn("MOON ROCK PARAMETERS OK", run.stdout)
            self.assertIn("THROWN ROCK EMPTIES HAND OK", run.stdout)
            self.assertIn("OWN LASER HIDDEN IN FIRST PERSON OK", run.stdout)
            self.assertIn("WAVE 3 OK", run.stdout)
            self.assertIn("ONE-SHOT LEVER HEADSHOTS OK", run.stdout)
            self.assertIn("COW BODY STOPS LASERS OK", run.stdout)
            self.assertIn("RAMP DESCENT OK", run.stdout)
            self.assertIn("ARRIVAL SOUND DOES NOT ALERT AI OK", run.stdout)
            self.assertIn("FINAL COMPLETION OK", run.stdout)
            self.assertIn("CLASSIC NO-STAGING COMPLETION OK", run.stdout)
            print(run.stdout, end="")


if __name__ == "__main__":
    unittest.main()
