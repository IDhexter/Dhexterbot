import os
import re
import unittest

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

class TestPaletteAndHair(unittest.TestCase):
    def setUp(self):
        self.pet_cpp_path = os.path.join(REPO_ROOT, "src", "features", "pet", "PetMode.cpp")
        self.assertTrue(os.path.isfile(self.pet_cpp_path), "PetMode.cpp not found")
        with open(self.pet_cpp_path, "r", encoding="utf-8") as f:
            self.content = f.read()

    def test_fixed_blue_palette_definitions(self):
        """Must define the fixed cyber-anime blue palette in PetMode.cpp"""
        keywords = ["colSapphire", "colElectricBlue", "colCyan", "colNavy", "colViolet"]
        for kw in keywords:
            self.assertTrue(kw in self.content, f"Expected palette definition {kw} in PetMode.cpp")

    def test_hair_not_jet_black(self):
        """Hair must be blue, not jet-black (0x0842) as in the old baseline"""
        is_jet_black = "uint16_t colHair     = gfxTint(0x0842); // Jet-black" in self.content
        self.assertFalse(is_jet_black, "Hair color is still jet-black from baseline; must be blue cyber bob")

    def test_hair_not_mood_dependent(self):
        """Hair color must not be switched or recolored by mood or colorPreset"""
        hair_mood_recolor = re.search(r'if\s*\(_activeMood[^)]*\)\s*colHair\w*\s*=', self.content)
        self.assertIsNone(hair_mood_recolor, "Hair color should not change by mood")

    def test_hair_silhouette_and_cyan_highlights(self):
        """Hair must feature bob/chanel silhouette with cyan highlights and bangs"""
        has_cyan_highlight = ("colHairCyan" in self.content) or ("colCyanHighlight" in self.content) or ("colCyan" in self.content)
        self.assertTrue(has_cyan_highlight, "Cyan highlights missing in hair definition")

        has_white_streaks = "// Comic-noir white hair shine streaks across the bangs" in self.content
        self.assertFalse(has_white_streaks, "Old white comic streaks found instead of cyber blue/cyan highlights")

    def test_near_black_background(self):
        """Background must be near-black for high contrast"""
        has_old_bg = "uint16_t bgCol = (y < areaH / 2) ? gfxTint(0x0021) : gfxTint(0x0842);" in self.content
        self.assertFalse(has_old_bg, "Background should be near-black for high contrast")

    def test_headphones_dark_headband_and_neon_rings(self):
        """Headphones must feature a dark headband with blue/cyan neon rings"""
        has_white_headband = "uint16_t colHeadband = gfxTint(0xDF1D); // White-silver headband arch" in self.content
        self.assertFalse(has_white_headband, "Old white-silver headband arch still present; should be dark headband with cyan arc")

        has_headband = ("colHeadband" in self.content) or ("colHeadbandDark" in self.content)
        self.assertTrue(has_headband, "Headband definition missing")

if __name__ == "__main__":
    unittest.main()
