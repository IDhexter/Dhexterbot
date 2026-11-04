# 🎧 DhexterBot 2.1 — NOC Cyber Girl (Super Expressiva) + Cérebro Neural & Hermes Bot

**DhexterBot 2.1** é um firmware autônomo em C++ desenvolvido sob medida para o hardware **GeekMagic SmallTV** (`ESP-12F / ESP8266`, 4 MB Flash, tela IPS `1.54" 240x240 ST7789`).

Transforma a SmallTV na **NOC Cyber Girl — Close-Up Anime (Super Expressiva)**: uma companheira de bancada e guardiã de rede com **Cérebro Neural Autônomo (LLM)**, **Login OAuth PKCE 1-Clique**, **Integração Bidirecional Nativa com o Hermes Agent (`hermes-agent`)** e monitoramento direto do **Zabbix NOC (`10.10.1.26`)**.

---

## ✨ Principais Recursos da v2.1

### 1. Visual "NOC Cyber Girl — Close-Up Anime (Super Expressiva)" em 240×240 (30 FPS)
- **Zero Barras de Fome/Level:** 100% da tela dedicada ao retrato anime super expressivo na sala de servidores e ao balão de diálogo cyberpunk de 2 linhas.
- **Personagem Fiel à Arte Original:**
  - **Cabelo Chanel Preto com Pontas Viradas para Cima:** contorno luminoso neon azul (`aura rim light`) e reflexos brancos de quadrinhos (*comic-noir*) na franja.
  - **Headphones Over-Ear com Arco Branco e Anel LED Neon Duplo:** o anel luminoso das conchas pulsa e muda de cor dinamicamente conforme o estado da rede (**Azul Ciano** normal, **Verde Esmeralda** tudo UP, **Roxo Neural** falando via IA/Hermes, **Rosa** carinho e **Vermelho Piscante** em queda de link!).
  - **Olhos Anime Safira em Camadas:** esclera branco-azulada, íris com degradê de 3 tons (azul-marinho → safira real → ciano luminoso), duplo brilho cristalino (*catchlights*), delineado superior com puxadinho (*winged eyeliner*), corações (`♥`), estrelas (`★`) ou anel de dados Persocom/Cyber!
  - **Fundo de Sala de Servidores (Datacenter NOC):** racks laterais de 19" com switches 1U e LEDs de portas piscando em tempo real.
  - **Lip-Sync em Tempo Real:** os lábios se movem sincronizados enquanto ela fala no balão inferior.

### 2. Cérebro Neural Autônomo + Login OAuth PKCE 1-Clique
- **Pensamentos Espontâneos Contextualizados:** periodicamente (ou com 1 clique na Web UI), o próprio ESP8266 consulta o LLM enviando horário NTP, temperatura externa ao vivo (`Open-Meteo`) e estado dos hosts do NOC (`Zabbix` / `Watchdog`), recebendo a frase e o humor em JSON (`~45 tokens` por chamada).
- **6 Provedores Neurais Suportados Direto no ESP8266 (`BearSSL`):**
  - `0` — **NVIDIA NIM (`nvidia/nemotron-3.5-super-vl-120b-a12b`)**: mesmo motor do Hermes Wazuh, pré-configurado de fábrica com `enable_thinking: false` para respostas instantâneas.
  - `1` — **Hermes Bot Gateway (`http://172.1.0.32:9119` / `172.17.0.40`)**: ponte HTTP direta com fallback automático para NVIDIA NIM.
  - `2` — **OpenRouter (Com Botão `🔗 Conectar via OAuth 1-Clique` na Web UI)**: fluxo OAuth PKCE completo direto no navegador!
  - `3` — **OpenAI Oficial (`gpt-4o-mini`)**
  - `4` — **Google Gemini (`gemini-2.0-flash` / `gemini-flash-lite-latest`)**
  - `5` — **Groq Cloud (`llama-3.3-70b-versatile`)**

### 3. Integração Bidirecional com o Hermes Bot (`/api/hermes/webhook`)
O seu **Hermes Agent** (rodando no Proxmox LXC 100 `172.1.0.32` ou no servidor Wazuh `172.17.0.40`) pode enviar alertas e conversar com a NOC Cyber Girl na sua mesa usando o endpoint `/api/hermes/webhook` ou o script [`scripts/hermes_dhexterbot_bridge.py`](scripts/hermes_dhexterbot_bridge.py):

```bash
# 1. Notificar nova vaga remota encontrada pelo Hermes Job Hunter:
python scripts/hermes_dhexterbot_bridge.py --event job_found --msg "Hermes: 2 vagas Sr Remotas salvas no Notion!"

# 2. Enviar alerta de segurança do Wazuh SIEM:
python scripts/hermes_dhexterbot_bridge.py --event wazuh_alert --msg "Wazuh SIEM: Alerta nivel 12 detectado!" --alerts 1

# 3. Fazer o Hermes conversar com o Cérebro Neural da SmallTV:
python scripts/hermes_dhexterbot_bridge.py --event ask --msg "Como estao os switches Aruba e links WAN?"
```

---

## 🛠️ Compilação & Atualização OTA (PlatformIO)

```bash
# Compilar o binário para ESP8266 (compacta src/webui.html automaticamente)
python -m platformio run -e smalltv

# Enviar via Wi-Fi OTA
curl.exe -X POST "http://10.0.15.235/update" -F "firmware=@.pio/build/smalltv/firmware.bin"
```
