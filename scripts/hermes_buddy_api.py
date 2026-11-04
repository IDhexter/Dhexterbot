#!/usr/bin/env python3
"""
hermes_buddy_api.py — API Pequena, Protegida e com Memória para o Hermes (Porta 9119)
Repositório: https://github.com/IDhexter/Dhexterbot

Arquitetura recomendada (ESP8266 = Corpo do Buddy | Hermes = Cérebro):
  - Nenhuma chave de IA (NVIDIA, OpenAI, Gemini) fica gravada no ESP8266.
  - Roda junto ao Hermes Agent (ex: no CT100 / LXC 100 na porta 9119).
  - Protege a comunicação entre o Pet e o Hermes via token (X-Buddy-Token / Bearer).
  - Mantém memória persistente e histórico de contexto em ~/.hermes/dhexterbot_memory.json.
  - Conecta-se de forma real ao Hermes Agent (lendo ~/.hermes/.env ou invocando o Hermes CLI).
  - Garante que todas as respostas caibam em no máximo 3 linhas de 18 caracteres (54 chars) sem acentos.
"""
import argparse
import json
import os
import re
import subprocess
import sys
import time
import unicodedata
import urllib.request
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path
from typing import Any, Dict, List, Optional

HERMES_DIR = Path.home() / ".hermes"
ENV_FILE = HERMES_DIR / ".env"
MEMORY_FILE = HERMES_DIR / "dhexterbot_memory.json"


def load_hermes_env() -> Dict[str, str]:
    """Load environment variables from ~/.hermes/.env if present."""
    env = dict(os.environ)
    if ENV_FILE.exists():
        for line in ENV_FILE.read_text(encoding="utf-8", errors="ignore").splitlines():
            line = line.strip()
            if not line or line.startswith("#") or "=" not in line:
                continue
            k, v = line.split("=", 1)
            k = k.strip()
            v = v.strip().strip('"').strip("'")
            if k and k not in env:
                env[k] = v
    return env


def strip_accents_ascii(text: str) -> str:
    """Convert accented characters to clean ASCII for the 240x240 ST7789 font."""
    nfkd = unicodedata.normalize("NFKD", text)
    ascii_str = "".join(c for c in nfkd if not unicodedata.combining(c))
    ascii_str = re.sub(r"[^\x20-\x7E]", "", ascii_str)
    ascii_str = re.sub(r"\s+", " ", ascii_str).strip()
    return ascii_str


def clamp_three_lines(text: str, max_lines: int = 3, line_width: int = 18) -> Dict[str, Any]:
    """Word-wrap and strictly clamp text to 3 lines of <= 18 chars (max 54 chars total)."""
    clean = strip_accents_ascii(text)
    max_total = max_lines * line_width
    words = clean.split(" ")
    lines: List[str] = []
    cur = ""
    for w in words:
        if not w:
            continue
        if len(w) > line_width:
            w = w[:line_width]
        candidate = f"{cur} {w}".strip() if cur else w
        if len(candidate) <= line_width:
            cur = candidate
        else:
            if cur:
                lines.append(cur)
            cur = w
            if len(lines) >= max_lines:
                break
    if cur and len(lines) < max_lines:
        lines.append(cur)
    joined = " ".join(lines)[:max_total].strip()
    return {"say": joined, "lines": lines[:max_lines]}


class BuddyMemory:
    """Persistent conversation and telemetry memory stored on the Hermes host."""

    def __init__(self, path: Path = MEMORY_FILE):
        self.path = path
        self.data: Dict[str, Any] = {
            "history": [],
            "last_state": {},
            "total_thoughts": 0,
        }
        self.load()

    def load(self) -> None:
        try:
            if self.path.exists():
                self.data = json.loads(self.path.read_text(encoding="utf-8"))
        except Exception:
            pass

    def save(self) -> None:
        try:
            self.path.parent.mkdir(parents=True, exist_ok=True)
            self.path.write_text(json.dumps(self.data, indent=2, ensure_ascii=False), encoding="utf-8")
        except Exception:
            pass

    def recent_summary(self, limit: int = 6) -> str:
        hist = self.data.get("history", [])[-limit:]
        if not hist:
            return "Nenhuma fala anterior."
        return " | ".join(f"[{h.get('time','')} {h.get('mood','')}]: {h.get('say','')}" for h in hist)

    def record(self, event: str, prompt: str, say: str, mood: str, telemetry: Dict[str, Any]) -> None:
        hist = self.data.setdefault("history", [])
        hist.append({
            "ts": int(time.time()),
            "time": telemetry.get("time", ""),
            "event": event,
            "prompt": prompt,
            "say": say,
            "mood": mood,
        })
        if len(hist) > 25:
            self.data["history"] = hist[-25:]
        self.data["last_state"] = telemetry
        self.data["total_thoughts"] = int(self.data.get("total_thoughts", 0)) + 1
        self.save()


def ask_hermes_brain(
    payload: Dict[str, Any],
    memory: BuddyMemory,
    env: Dict[str, str],
) -> Dict[str, str]:
    """Query Hermes Agent on CT100 (via configured LLM in ~/.hermes/.env or hermes CLI)."""
    persona = payload.get("persona") or "NOC Cyber Girl carismatica, prestativa e bem-humorada"
    event = payload.get("event") or "autonomous"
    user_prompt = (payload.get("prompt") or "").strip()
    time_str = payload.get("time") or "agora"
    temp_c = payload.get("tempC", 26.0)
    hosts_up = payload.get("hostsUp", 0)
    hosts_down = payload.get("hostsDown", 0)
    zbx_alerts = payload.get("zbxAlerts", 0)

    recent = memory.recent_summary(5)

    if user_prompt:
        task_instruction = f'O usuario perguntou/disse: "{user_prompt}". Responda diretamente a ele.'
    else:
        task_instruction = (
            "Gere um comentario espontaneo inedito (diferente do historico recente) "
            "sobre o estado da rede, horario ou clima."
        )

    sys_prompt = (
        f"Voce e DhexterBot ({persona}), o corpo fisico do Hermes na bancada (tela 240x240).\n"
        f"Contexto ao vivo: Hora={time_str}, Clima={temp_c:.1f}C, HostsUP={hosts_up}, "
        f"HostsDown={hosts_down}, ZabbixAlerts={zbx_alerts}.\n"
        f"Memoria recente (NAO repita frases): {recent}\n"
        f"{task_instruction}\n"
        "REGRAS CRITICAS: Responda em Portugues SEM acentos, no maximo 48 caracteres (3 linhas curtas). "
        'Retorne APENAS JSON valido: {"mood":"happy|normal|curious|excited|cool|cyber|angry|love","action":"none|coffee|pet|charge|hacker|celebrate","say":"frase curta"}'
    )

    raw_reply = ""

    # 1. Use Gemini if GEMINI_API_KEY / GOOGLE_API_KEY is configured in ~/.hermes/.env
    gem_key = env.get("GEMINI_API_KEY") or env.get("GOOGLE_API_KEY")
    nv_key = env.get("NVIDIA_API_KEY") or env.get("NVAPI_KEY")
    oa_key = env.get("OPENAI_API_KEY")
    or_key = env.get("OPENROUTER_API_KEY")

    try:
        if gem_key:
            model = env.get("HERMES_GEMINI_MODEL", "gemini-2.0-flash")
            url = f"https://generativelanguage.googleapis.com/v1beta/models/{model}:generateContent?key={gem_key}"
            req_body = {"contents": [{"parts": [{"text": sys_prompt}]}]}
            req = urllib.request.Request(
                url,
                data=json.dumps(req_body).encode("utf-8"),
                headers={"Content-Type": "application/json"},
                method="POST",
            )
            with urllib.request.urlopen(req, timeout=8) as r:
                d = json.loads(r.read().decode("utf-8"))
                raw_reply = d["candidates"][0]["content"]["parts"][0]["text"]
        elif nv_key or oa_key or or_key:
            if nv_key:
                url = "https://integrate.api.nvidia.com/v1/chat/completions"
                key = nv_key
                model = env.get("HERMES_MODEL", "nvidia/nemotron-3.5-super-vl-120b-a12b")
                extra = {"chat_template_kwargs": {"enable_thinking": False}}
            elif or_key:
                url = "https://openrouter.ai/api/v1/chat/completions"
                key = or_key
                model = env.get("HERMES_MODEL", "openai/gpt-4o-mini")
                extra = {}
            else:
                url = "https://api.openai.com/v1/chat/completions"
                key = oa_key
                model = env.get("HERMES_MODEL", "gpt-4o-mini")
                extra = {}

            req_body = {
                "model": model,
                "max_tokens": 90,
                "temperature": 0.75,
                "messages": [{"role": "user", "content": sys_prompt}],
                **extra,
            }
            req = urllib.request.Request(
                url,
                data=json.dumps(req_body).encode("utf-8"),
                headers={
                    "Content-Type": "application/json",
                    "Authorization": f"Bearer {key}",
                },
                method="POST",
            )
            with urllib.request.urlopen(req, timeout=8) as r:
                d = json.loads(r.read().decode("utf-8"))
                raw_reply = d["choices"][0]["message"]["content"]
        else:
            # Fallback to local hermes CLI if installed in PATH
            proc = subprocess.run(
                ["hermes", "chat", "-q", sys_prompt],
                capture_output=True,
                text=True,
                timeout=10,
            )
            if proc.returncode == 0 and proc.stdout.strip():
                raw_reply = proc.stdout.strip()
    except Exception as exc:
        print(f"[hermes-buddy-api] LLM call warning: {exc}", file=sys.stderr)

    mood = "cyber"
    action = ""
    say_text = raw_reply.strip()

    if "{" in say_text and "}" in say_text:
        try:
            s_idx = say_text.find("{")
            e_idx = say_text.rfind("}")
            parsed = json.loads(say_text[s_idx : e_idx + 1])
            say_text = parsed.get("say") or parsed.get("reply") or say_text
            mood = parsed.get("mood") or mood
            act = parsed.get("action") or ""
            if act and act != "none":
                action = act
        except Exception:
            pass

    if not say_text:
        if hosts_down > 0 or zbx_alerts > 0:
            mood = "angry"
            say_text = f"Alerta NOC: {hosts_down + zbx_alerts} ativo(s) requerem atencao!"
        else:
            mood = "happy"
            say_text = f"Hermes CT100 ativo! Rede 100% UP ({temp_c:.0f}C)."

    clamped = clamp_three_lines(say_text, max_lines=3, line_width=18)
    memory.record(event, user_prompt, clamped["say"], mood, payload)

    return {
        "ok": True,
        "mood": mood,
        "action": action,
        "say": clamped["say"],
        "lines": clamped["lines"],
    }


class BuddyRequestHandler(BaseHTTPRequestHandler):
    buddy_token: str = ""
    pet_ip: str = "10.0.15.235"
    memory: BuddyMemory = BuddyMemory()
    env: Dict[str, str] = {}

    def _check_auth(self, body_obj: Optional[Dict[str, Any]] = None) -> bool:
        if not self.buddy_token:
            return True
        hdr = self.headers.get("X-Buddy-Token", "")
        auth = self.headers.get("Authorization", "")
        if hdr == self.buddy_token or auth == self.buddy_token or auth == f"Bearer {self.buddy_token}":
            return True
        if body_obj and body_obj.get("token") == self.buddy_token:
            return True
        return False

    def _send_json(self, code: int, obj: Dict[str, Any]) -> None:
        raw = json.dumps(obj, ensure_ascii=False).encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(raw)))
        self.end_headers()
        self.wfile.write(raw)

    def do_GET(self) -> None:
        if not self._check_auth():
            self._send_json(401, {"ok": False, "error": "unauthorized"})
            return
        self._send_json(
            200,
            {
                "ok": True,
                "service": "hermes-buddy-api",
                "port": self.server.server_port,
                "authProtected": bool(self.buddy_token),
                "totalThoughts": self.memory.data.get("total_thoughts", 0),
                "lastState": self.memory.data.get("last_state", {}),
                "recentHistory": self.memory.data.get("history", [])[-5:],
            },
        )

    def do_POST(self) -> None:
        length = int(self.headers.get("Content-Length", "0") or 0)
        raw_body = self.rfile.read(length).decode("utf-8", errors="ignore") if length > 0 else "{}"
        try:
            body_obj = json.loads(raw_body) if raw_body else {}
        except Exception:
            body_obj = {}

        if not self._check_auth(body_obj):
            self._send_json(401, {"ok": False, "error": "unauthorized"})
            return

        if self.path.startswith("/api/buddy/push"):
            # Push a 3-line formatted message from Hermes directly to the ESP8266 Pet
            msg = body_obj.get("message") or body_obj.get("say") or "Hermes: Notificacao!"
            clamped = clamp_three_lines(msg)
            pet_url = f"http://{self.pet_ip}/api/hermes/webhook"
            out_payload = {
                "event": body_obj.get("event", "notify"),
                "message": clamped["say"],
                "mood": body_obj.get("mood", "cyber"),
                "action": body_obj.get("action", ""),
                "alerts": body_obj.get("alerts", -1),
            }
            headers = {"Content-Type": "application/json"}
            if self.buddy_token:
                headers["X-Buddy-Token"] = self.buddy_token
            req = urllib.request.Request(
                pet_url,
                data=json.dumps(out_payload).encode("utf-8"),
                headers=headers,
                method="POST",
            )
            try:
                with urllib.request.urlopen(req, timeout=6) as resp:
                    pet_res = json.loads(resp.read().decode("utf-8"))
                self._send_json(200, {"ok": True, "pushed": out_payload, "pet": pet_res})
            except Exception as exc:
                self._send_json(502, {"ok": False, "error": str(exc)})
            return

        # Default: /api/buddy/think or /api/chat
        result = ask_hermes_brain(body_obj, self.memory, self.env)
        self._send_json(200, result)


def main() -> None:
    env = load_hermes_env()
    parser = argparse.ArgumentParser(description="Hermes Buddy Protected API Server (Port 9119)")
    parser.add_argument("--host", default="0.0.0.0", help="Bind address (default: 0.0.0.0)")
    parser.add_argument("--port", type=int, default=int(env.get("BUDDY_PORT", "9119")), help="Port (default: 9119)")
    parser.add_argument(
        "--token",
        default=env.get("BUDDY_TOKEN", ""),
        help="Shared authentication token (X-Buddy-Token) between ESP8266 Pet and Hermes",
    )
    parser.add_argument(
        "--pet-ip",
        default=env.get("DHEXTERBOT_IP", "10.0.15.235"),
        help="ESP8266 DhexterBot IP for push notifications (default: 10.0.15.235)",
    )
    args = parser.parse_args()

    BuddyRequestHandler.buddy_token = args.token
    BuddyRequestHandler.pet_ip = args.pet_ip
    BuddyRequestHandler.env = env

    server = HTTPServer((args.host, args.port), BuddyRequestHandler)
    print(
        f"[hermes-buddy-api] Listening on http://{args.host}:{args.port} "
        f"(auth={'ENABLED' if args.token else 'OPEN'}, memory={MEMORY_FILE})"
    )
    server.serve_forever()


if __name__ == "__main__":
    main()
