import os
import re
import unittest

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

class TestSecuritySecrets(unittest.TestCase):
    def test_settings_no_hardcoded_api_keys(self):
        settings_path = os.path.join(REPO_ROOT, "src", "Settings.cpp")
        self.assertTrue(os.path.isfile(settings_path), "Settings.cpp not found")

        with open(settings_path, "r", encoding="utf-8") as f:
            content = f.read()

        # Check for live-looking key patterns
        p_nv = "nv" + "api-[A-Za-z0-9_-]{20,}"
        p_sk = "s" + "k-[A-Za-z0-9]{20,}"
        p_gh = "gh" + "p_[A-Za-z0-9]{20,}"
        p_ak = "AK" + "IA[0-9A-Z]{16}"
        p_ai = "AI" + "za[0-9A-Za-z-_]{35}"
        patterns = [p_nv, p_sk, p_gh, p_ak, p_ai]
        for pattern in patterns:
            has_secret = bool(re.search(pattern, content))
            self.assertFalse(has_secret, f"Found pattern matching secret in Settings.cpp: {pattern}")

    def test_settings_uses_hermes_only_with_empty_token(self):
        settings_path = os.path.join(REPO_ROOT, "src", "Settings.cpp")
        with open(settings_path, "r", encoding="utf-8") as f:
            content = f.read()

        self.assertNotRegex(
            content,
            r'\baiKey\s*\[',
            "Cloud AI key storage must not return to ESP settings",
        )
        self.assertNotRegex(
            content,
            r'\baiEnabled\s*=\s*true\s*;',
            "Cloud AI must not be enabled directly on the ESP",
        )
        self.assertRegex(
            content,
            r'hermesToken\[0\]\s*=\s*0\s*;',
            "Hermes mutual-auth token must default to an empty value",
        )

    def test_whole_src_no_secrets(self):
        src_dir = os.path.join(REPO_ROOT, "src")
        p_nv = "nv" + "api-[A-Za-z0-9_-]{20,}"
        p_sk = "s" + "k-[A-Za-z0-9_-]{20,}"
        p_gh = "gh" + "(?:o_|p_)[A-Za-z0-9_]{20,}"
        p_ak = "AK" + "IA[0-9A-Z]{16}"
        p_ai = "AI" + "za[0-9A-Za-z-_]{35}"
        secret_regex = re.compile(f'({p_nv}|{p_sk}|{p_gh}|{p_ak}|{p_ai})')

        for root, _, files in os.walk(src_dir):
            for file in files:
                if file.endswith((".cpp", ".h", ".c", ".hpp")):
                    path = os.path.join(root, file)
                    with open(path, "r", encoding="utf-8", errors="ignore") as f:
                        text = f.read()
                    has_secret = bool(secret_regex.search(text))
                    self.assertFalse(has_secret, f"Secret pattern matched in {path}")

if __name__ == "__main__":
    unittest.main()
