import os
import re
import unittest

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


class TestDialogText(unittest.TestCase):
    def setUp(self):
        cpp_path = os.path.join(REPO_ROOT, "src", "features", "pet", "PetMode.cpp")
        h_path = os.path.join(REPO_ROOT, "src", "features", "pet", "PetMode.h")
        with open(cpp_path, "r", encoding="utf-8") as f:
            self.cpp = f.read()
        with open(h_path, "r", encoding="utf-8") as f:
            self.header = f.read()

    def test_long_dialog_is_preserved_and_paginated(self):
        """Long replies must be retained and rendered as successive three-line pages."""
        speech_size = re.search(r"_speechText\[(\d+)\]", self.header)
        reply_size = re.search(r"_lastAiReply\[(\d+)\]", self.header)
        self.assertIsNotNone(speech_size)
        self.assertIsNotNone(reply_size)
        self.assertGreaterEqual(int(speech_size.group(1)), 192)
        self.assertGreaterEqual(int(reply_size.group(1)), 192)
        self.assertNotIn("j < 54", self.cpp)
        self.assertIn("_dialogPage", self.header)
        self.assertIn("_nextDialogPageMs", self.header)
        self.assertRegex(self.cpp, r"printWrappedBig\([^;]+_dialogPage")

    def test_portuguese_utf8_glyphs_are_rendered(self):
        """Portuguese accents must be decoded and drawn instead of dropped or printed raw."""
        self.assertIn("decodePortugueseGlyph", self.cpp)
        self.assertIn("drawPortugueseText", self.cpp)
        for mark in ("ACCENT_ACUTE", "ACCENT_GRAVE", "ACCENT_CIRCUMFLEX", "ACCENT_TILDE", "ACCENT_CEDILLA"):
            self.assertIn(mark, self.cpp)
        # UTF-8 second bytes for ã/õ and ç must have explicit mappings.
        self.assertRegex(self.cpp, r"0xA3[^\n]+ACCENT_TILDE")
        self.assertRegex(self.cpp, r"0xB5[^\n]+ACCENT_TILDE")
        self.assertRegex(self.cpp, r"0xA7[^\n]+ACCENT_CEDILLA")
        # Every speech-bearing view must use glyph-aware rendering.
        self.assertGreaterEqual(self.cpp.count("drawPortugueseText("), 4)
        self.assertNotRegex(self.cpp, r"strlcpy\(clip,\s*botMsg")
        self.assertNotRegex(self.cpp, r"strlcpy\(sumBuf,\s*_speechText")

    def test_external_alert_does_not_reintroduce_a_short_buffer(self):
        """External alert text must reach the 192-byte speech store without a 68-byte hop."""
        match = re.search(
            r"void PetMode::pushExternalAlert\([^)]*\)\s*\{(?P<body>.*?)\n\}",
            self.cpp,
            re.S,
        )
        self.assertIsNotNone(match)
        body = match.group("body") if match is not None else ""
        self.assertNotRegex(body, r"char\s+msg\[68\]")
        self.assertRegex(body, r"triggerSay\(message,")


if __name__ == "__main__":
    unittest.main()
