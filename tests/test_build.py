import os
import shutil
import subprocess
import unittest

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

class TestBuild(unittest.TestCase):
    def test_platformio_smalltv_build(self):
        """Firmware for smalltv must compile with zero errors"""
        pio_bin = os.path.expanduser("~/.local/bin/pio")
        if not os.path.isfile(pio_bin):
            pio_bin = shutil.which("pio")
        if not pio_bin:
            self.skipTest("PlatformIO 'pio' executable is not installed")

        cmd = [pio_bin, "run", "-e", "smalltv"]
        proc = subprocess.run(
            cmd,
            cwd=REPO_ROOT,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            timeout=600
        )
        self.assertEqual(
            proc.returncode,
            0,
            f"PlatformIO build failed with output:\n{proc.stdout}"
        )

        bin_path = os.path.join(REPO_ROOT, ".pio", "build", "smalltv", "firmware.bin")
        self.assertTrue(os.path.isfile(bin_path), f"Output binary not found at {bin_path}")
        elf_path = os.path.join(REPO_ROOT, ".pio", "build", "smalltv", "firmware.elf")
        self.assertTrue(os.path.isfile(elf_path), f"Output ELF not found at {elf_path}")

if __name__ == "__main__":
    unittest.main()
