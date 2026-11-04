#!/usr/bin/env python3
"""
hermes_dhexterbot_bridge.py — Conector Oficial Hermes Agent <-> DhexterBot (NOC Cyber Girl)
Repositório: https://github.com/IDhexter/Dhexterbot

Permite que o Hermes Agent (Proxmox LXC 100 / Wazuh SOC / Job Hunter) controle
a NOC Cyber Girl na tela física da SmallTV (http://10.0.15.235):
  - Enviar alertas do Wazuh SIEM (fones ficam vermelhos + expressão de alerta)
  - Notificar novas vagas remotas salvas no Notion pelo Job Hunter (olhos de estrela ★)
  - Sincronizar incidentes do Zabbix NOC (10.10.1.26)
  - Conversar diretamente com o Cérebro Neural da SmallTV
"""
import argparse
import json
import sys
import urllib.request

DEFAULT_DHEXTERBOT_IP = "10.0.15.235"


def send_hermes_webhook(ip: str, event: str, message: str, mood: str = "", alerts: int = -1) -> dict:
    url = f"http://{ip}/api/hermes/webhook"
    payload = {
        "event": event,
        "message": message,
    }
    if mood:
        payload["mood"] = mood
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


def main():
    parser = argparse.ArgumentParser(description="Hermes Bot <-> DhexterBot NOC Cyber Girl Bridge")
    parser.add_argument("--ip", default=DEFAULT_DHEXTERBOT_IP, help="IP do DhexterBot na rede (padrao: 10.0.15.235)")
    parser.add_argument(
        "--event",
        default="say",
        choices=["wazuh_alert", "job_found", "zabbix_ok", "telegram", "say", "ask"],
        help="Tipo de evento enviado pelo Hermes Bot",
    )
    parser.add_argument("--msg", required=True, help="Mensagem ou pergunta para exibir na SmallTV")
    parser.add_argument("--mood", default="", help="Expressao opcional (happy, excited, cyber, angry, cool, love)")
    parser.add_argument("--alerts", type=int, default=-1, help="Contagem de incidentes ativos no NOC/Wazuh")

    args = parser.parse_args()
    try:
        res = send_hermes_webhook(args.ip, args.event, args.msg, args.mood, args.alerts)
        print(json.dumps(res, indent=2, ensure_ascii=False))
    except Exception as exc:
        print(f"Erro ao comunicar com DhexterBot ({args.ip}): {exc}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
