"""Run the web/net node:test suite from the Python test harness.

Skips (instead of failing) if Node.js isn't installed, so the unittest
gate stays green on machines without Node. When Node is present, this
must pass for the gate in docs/ONLINE-PLAN.md.
"""
import glob
import shutil
import subprocess
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
TEST_DIR = REPO_ROOT / 'tests' / 'web-net'


class WebNetTests(unittest.TestCase):
    def test_node_test_suite_passes(self):
        node = shutil.which('node')
        if not node:
            self.skipTest('node is not installed')

        test_files = sorted(glob.glob(str(TEST_DIR / '*.test.mjs')))
        self.assertTrue(test_files, f'no *.test.mjs files found under {TEST_DIR}')

        # Pass explicit file globs rather than the bare directory: Node's
        # test runner treats an explicit directory argument as a single
        # module to require (and fails) rather than recursing into it the
        # way its *implicit* default-directory discovery does.
        result = subprocess.run(
            [node, '--test', *test_files],
            cwd=REPO_ROOT,
            capture_output=True,
            text=True,
            timeout=120,
        )
        self.assertEqual(
            result.returncode,
            0,
            f'node --test failed (exit {result.returncode}):\n'
            f'--- stdout ---\n{result.stdout}\n--- stderr ---\n{result.stderr}',
        )


if __name__ == '__main__':
    unittest.main()
