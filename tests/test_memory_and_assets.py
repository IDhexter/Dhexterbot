import os
import re
import unittest

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

class TestMemoryAndAssets(unittest.TestCase):
    def setUp(self):
        self.pet_cpp_path = os.path.join(REPO_ROOT, "src", "features", "pet", "PetMode.cpp")
        with open(self.pet_cpp_path, "r", encoding="utf-8") as f:
            self.cpp_content = f.read()

    def test_no_full_framebuffer(self):
        """Must not allocate a full 240x240 framebuffer (115KB would exhaust ESP8266 RAM)"""
        # Search for full screen buffer arrays or allocations
        forbidden_patterns = [
            r'uint16_t\s+\w+\[240\s*\*\s*240\]',
            r'uint16_t\s+\w+\[57600\]',
            r'malloc\(\s*240\s*\*\s*240',
            r'malloc\(\s*115200',
            r'new\s+uint16_t\[240\s*\*\s*240\]',
        ]
        for pat in forbidden_patterns:
            self.assertIsNone(
                re.search(pat, self.cpp_content),
                f"Forbidden full framebuffer pattern found: {pat}"
            )

    def test_uses_scanline_buffer(self):
        """Should use a 1-line scanline buffer (rowBuf) of width 240"""
        self.assertRegex(
            self.cpp_content,
            r'uint16_t\s+rowBuf\[areaW\];',
            "Expected scanline buffer rowBuf[areaW] in PetMode.cpp"
        )

    def test_no_dynamic_allocation_in_draw_loop(self):
        """No malloc, calloc, or new in drawFaceRegion"""
        # Extract drawFaceRegion method body
        match = re.search(
            r'void\s+PetMode::drawFaceRegion\([^)]*\)\s*\{([\s\S]*?)\n\}\n',
            self.cpp_content
        )
        self.assertIsNotNone(match, "drawFaceRegion function body not found")
        draw_body = match.group(1)

        forbidden_tokens = ["malloc(", "calloc(", "realloc(", "new ", "strdup("]
        for token in forbidden_tokens:
            self.assertNotIn(
                token,
                draw_body,
                f"Dynamic memory allocation token '{token}' forbidden inside drawFaceRegion"
            )

    def test_no_large_embedded_asset_arrays(self):
        """No large embedded bitmap/image arrays (>8KB) in PetMode"""
        pet_dir = os.path.join(REPO_ROOT, "src", "features", "pet")
        for fname in os.listdir(pet_dir):
            fpath = os.path.join(pet_dir, fname)
            if os.path.isfile(fpath) and fname.endswith((".cpp", ".h")):
                with open(fpath, "r", encoding="utf-8", errors="ignore") as f:
                    text = f.read()
                # Check for large hex arrays like {0x00, 0x01, ...} of thousands of items
                arrays = re.findall(r'const\s+uint(?:8|16)_t\s+\w+\[\s*(\d+)\s*\]', text)
                for arr_size_str in arrays:
                    arr_size = int(arr_size_str)
                    self.assertLess(
                        arr_size,
                        4096,
                        f"Found large asset array of size {arr_size} in {fname}, violates embedded asset restriction"
                    )

if __name__ == "__main__":
    unittest.main()
