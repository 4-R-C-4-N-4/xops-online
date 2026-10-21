import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC_DIR = ROOT / 'src'
TEST_CPP = ROOT / 'tests' / 'net' / 'protocol_test.cpp'
PROTOCOL_CPP = ROOT / 'src' / 'net' / 'protocol.cpp'


@unittest.skipIf(shutil.which('g++') is None, 'g++ not available')
class NetProtocolTest(unittest.TestCase):
    def test_protocol_roundtrip_and_fuzz(self):
        with tempfile.TemporaryDirectory() as tmp:
            binary = Path(tmp) / 'protocol_test'
            cmd = [
                'g++', '-std=c++17', '-O1', '-fsanitize=address,undefined',
                '-I', str(SRC_DIR),
                str(TEST_CPP), str(PROTOCOL_CPP),
                '-o', str(binary),
            ]
            build = subprocess.run(cmd, capture_output=True, text=True)
            self.assertEqual(
                build.returncode, 0,
                msg=f'build failed:\nstdout={build.stdout}\nstderr={build.stderr}',
            )

            run = subprocess.run([str(binary)], capture_output=True, text=True)
            self.assertEqual(
                run.returncode, 0,
                msg=f'test binary failed:\nstdout={run.stdout}\nstderr={run.stderr}',
            )
            self.assertIn('NET PROTOCOL OK', run.stdout)


if __name__ == '__main__':
    unittest.main()
