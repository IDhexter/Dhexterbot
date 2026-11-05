import os
import re
import unittest

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

class TestStatesAndAnimations(unittest.TestCase):
    def setUp(self):
        self.pet_h_path = os.path.join(REPO_ROOT, "src", "features", "pet", "PetMode.h")
        self.pet_cpp_path = os.path.join(REPO_ROOT, "src", "features", "pet", "PetMode.cpp")
        with open(self.pet_h_path, "r", encoding="utf-8") as f:
            self.h_content = f.read()
        with open(self.pet_cpp_path, "r", encoding="utf-8") as f:
            self.cpp_content = f.read()

    def test_mandatory_mood_states_exist(self):
        """All mandatory mood states must be defined in PetMood enum"""
        mandatory_moods = [
            "PET_MOOD_NORMAL",
            "PET_MOOD_HAPPY",
            "PET_MOOD_CURIOUS",
            "PET_MOOD_EXCITED",
            "PET_MOOD_COOL",
            "PET_MOOD_CYBER",
            "PET_MOOD_ANGRY",
            "PET_MOOD_LOVE",
        ]
        for mood in mandatory_moods:
            self.assertIn(mood, self.h_content, f"Missing required mood {mood} in PetMode.h")

    def test_blink_logic_present(self):
        """Blink animation must handle open/closed state for both eyes"""
        self.assertIn("_openL", self.h_content)
        self.assertIn("_openR", self.h_content)
        self.assertIn("_blinkStage", self.h_content)
        self.assertIn("triggerBlink()", self.h_content)
        # Check rendering accounts for eyelid closure
        self.assertIn("_openL", self.cpp_content)

    def test_gaze_saccade_movement(self):
        """Eye gaze / saccade movement physics must be active"""
        self.assertIn("_curX", self.h_content)
        self.assertIn("_curY", self.h_content)
        self.assertIn("_velX", self.h_content)
        self.assertIn("_velY", self.h_content)
        self.assertIn("updatePhysics", self.cpp_content)

    def test_eyebrows_by_mood(self):
        """Eyebrows must adjust based on active mood (e.g. angry, curious, happy)"""
        self.assertIn("PET_MOOD_ANGRY", self.cpp_content)
        self.assertIn("PET_MOOD_CURIOUS", self.cpp_content)
        # Check that eyebrow rendering exists, varies with mood, and is drawn once
        # after the fringe so it remains visible without duplicate per-frame work.
        self.assertRegex(self.cpp_content, r'browY\s*=')
        self.assertEqual(self.cpp_content.count("// Eyebrows overlay across fringe"), 1)
        self.assertNotIn("// Eyebrows by mood", self.cpp_content)

    def test_mouth_lip_sync(self):
        """Mouth animation and lip-sync must react when talking"""
        self.assertIn("talking", self.cpp_content)
        self.assertIn("talkH", self.cpp_content)
        self.assertIn("showMouth", self.cpp_content)

    def test_headphone_pulsation(self):
        """Headphones must feature pulse animation"""
        self.assertIn("pulseBright", self.cpp_content)
        self.assertIn("colRingActive", self.cpp_content)

    def test_two_catchlights_and_layered_iris(self):
        """Eye rendering must include two catchlights and layered iris"""
        self.assertIn("c1x", self.cpp_content)
        self.assertIn("c2x", self.cpp_content)
        # Sclera, iris and pupil presence
        self.assertIn("colSclera", self.cpp_content)

    def test_no_full_screen_redraw_per_frame(self):
        """Full screen redraw should not be invoked every frame, only when invalidated"""
        # _fullRepaint must be used conditionally in service()
        self.assertIn("if (_fullRepaint)", self.cpp_content)
        self.assertIn("_fullRepaint = false;", self.cpp_content)

if __name__ == "__main__":
    unittest.main()
