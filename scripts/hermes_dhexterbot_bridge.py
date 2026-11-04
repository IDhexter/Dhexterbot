#!/usr/bin/env python3
"""
hermes_dhexterbot_bridge.py — Universal Hermes Agent Skill & CLI Bridge for DhexterBot
Repository: https://github.com/IDhexter/Dhexterbot

Provides 100% universal integration between ANY Hermes Agent instance
(Nous Research hermes-agent, OpenAI-compatible agent, MCP server, or CLI)
and the physical DhexterBot (NOC Cyber Girl) on ESP8266 SmallTV:
  1. Can be imported directly as a Python Tool/Skill inside any Hermes Agent.
  2. Can be executed from the command line or any automation/webhook.
  3. Supports auto-discovery via GET /api/hermes and control via POST /api/hermes/webhook.
"""
import argparse
import json
import os
import sys
import urllib.request
from typing import Any, Dict, Optional

DEFAULT_DHEXTERBOT_HOST = os.environ.get("DHEXTERBOT_HOST", "dhexterbot.local")

# Universal OpenAI / Hermes Agent Function-Calling Tool Specification
HERMES_TOOL_SCHEMA: Dict[str, Any] = {
    "type": "function",
    "function": {
        "name": "dhexterbot_control",
        "description": (
            "Control the physical DhexterBot (NOC Cyber Girl anime companion on SmallTV 240x240 display). "
            "Use this tool to speak messages on her screen, change her facial expression/mood, "
            "trigger animations, push alert counts, or ask her autonomous neural brain."
        ),
        "parameters": {
            "type": "object",
            "properties": {
                "event": {
                    "type": "string",
                    "enum": ["notify", "success", "alert", "ok", "ask"],
                    "description": "Type of interaction: 'notify' (display message), 'success' (celebrate task done), 'alert' (critical red headphone pulse), 'ok' (all clear), or 'ask' (converse with her neural LLM).",
                },
                "message": {
                    "type": "string",
                    "description": "Text message to display in the big 3-line dialogue window (max 54 chars recommended).",
                },
                "mood": {
                    "type": "string",
                    "enum": ["happy", "excited", "cyber", "cool", "curious", "angry", "love", "normal", "tired", "sleep"],
                    "description": "Optional anime facial expression to display.",
                },
                "action": {
                    "type": "string",
                    "enum": ["coffee", "pet", "charge", "hacker", "celebrate"],
                    "description": "Optional physical animation to trigger on screen.",
                },
                "alerts": {
                    "type": "integer",
                    "description": "Optional active alert counter (>=1 turns headphone rings flashing red; 0 clears alerts).",
                },
            },
            "required": ["event", "message"],
        },
    },
}


def get_dhexterbot_status(host: str = DEFAULT_DHEXTERBOT_HOST) -> Dict[str, Any]:
    """Query live status & Universal Hermes schema from DhexterBot."""
    base = host if host.startswith("http") else f"http://{host}"
    url = f"{base.rstrip('/')}/api/hermes"
    req = urllib.request.Request(url, method="GET")
    with urllib.request.urlopen(req, timeout=8) as resp:
        return json.loads(resp.read().decode("utf-8"))


def dhexterbot_control(
    message: str,
    event: str = "notify",
    mood: str = "",
    action: str = "",
    alerts: int = -1,
    host: str = DEFAULT_DHEXTERBOT_HOST,
) -> Dict[str, Any]:
    """Send a universal Hermes event, speech, mood, or neural question to DhexterBot."""
    base = host if host.startswith("http") else f"http://{host}"
    url = f"{base.rstrip('/')}/api/hermes/webhook"
    payload: Dict[str, Any] = {
        "event": event,
        "message": message,
    }
    if mood:
        payload["mood"] = mood
    if action:
        payload["action"] = action
    if alerts >= 0:
        payload["alerts"] = alerts

    req = urllib.request.Request(
        url,
        data=json.dumps(payload).encode("utf-8"),
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    with urllib.request.urlopen(req, timeout=10) as resp:
        return json.loads(resp.read().decode("utf-8"))


def main() -> None:
    parser = argparse.ArgumentParser(description="Universal Hermes Agent <-> DhexterBot Bridge")
    parser.add_argument(
        "--host",
        "--ip",
        dest="host",
        default=DEFAULT_DHEXTERBOT_HOST,
        help="Hostname or IP of DhexterBot (default: dhexterbot.local or $DHEXTERBOT_HOST)",
    )
    parser.add_argument("--status", action="store_true", help="Fetch live status & Hermes tool manifest (GET /api/hermes)")
    parser.add_argument(
        "--event",
        default="notify",
        choices=["notify", "success", "alert", "ok", "ask", "say"],
        help="Universal Hermes event type",
    )
    parser.add_argument("--msg", default="", help="Message or question for DhexterBot")
    parser.add_argument("--mood", default="", help="Optional mood (happy, excited, cyber, cool, curious, angry, love, normal, sleep)")
    parser.add_argument("--action", default="", help="Optional action (coffee, pet, charge, hacker, celebrate)")
    parser.add_argument("--alerts", type=int, default=-1, help="Optional alert counter")

    args = parser.parse_args()
    try:
        if args.status:
            res = get_dhexterbot_status(args.host)
        else:
            if not args.msg:
                parser.error("--msg is required unless --status is used")
            res = dhexterbot_control(
                message=args.msg,
                event=args.event,
                mood=args.mood,
                action=args.action,
                alerts=args.alerts,
                host=args.host,
            )
        print(json.dumps(res, indent=2, ensure_ascii=False))
    except Exception as exc:
        print(f"Error communicating with DhexterBot ({args.host}): {exc}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
