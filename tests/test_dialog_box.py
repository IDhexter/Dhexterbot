import os
import re
import unittest

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

class TestDialogBox(unittest.TestCase):
    def setUp(self):
        self.pet_cpp_path = os.path.join(REPO_ROOT, "src", "features", "pet", "PetMode.cpp")
        with open(self.pet_cpp_path, "r", encoding="utf-8") as f:
            self.cpp_content = f.read()

    def test_face_regions_stop_before_each_lower_panel(self):
        """Every view's face region must stop before its corresponding lower panel."""
        service = re.search(
            r"void PetMode::service\(const Settings& s\)\s*\{(?P<body>.*)\n\}",
            self.cpp_content,
            re.S,
        )
        if service is None:
            self.fail("PetMode::service() not found")
        calls = [
            (int(y), int(h))
            for y, h in re.findall(
                r"drawFaceRegion\s*\(\s*s\s*,\s*(\d+)\s*,\s*(\d+)\s*\)",
                service.group("body"),
            )
        ]
        self.assertEqual(calls, [(23, 103), (23, 99), (23, 137)])
        for (area_y, area_h), panel_start in zip(calls, (130, 124, 160)):
            self.assertLessEqual(
                area_y + area_h,
                panel_start,
                f"Face region {area_y}+{area_h} overlaps panel at {panel_start}",
            )

    def test_dialog_box_drawing_function_preserved(self):
        """drawBottomPanel must exist and draw round rect dialogue bubble in lower region"""
        self.assertIn("void PetMode::drawBottomPanel", self.cpp_content)
        self.assertIn("fillRoundRect(4, 161, 232, 77", self.cpp_content)
        self.assertIn("drawRoundRect(4, 161, 232, 77", self.cpp_content)

    def test_text_wrapping_in_dialog_box(self):
        """Text printing in dialog box must wrap long speech lines"""
        self.assertIn("printWrappedBig", self.cpp_content)

if __name__ == "__main__":
    unittest.main()
