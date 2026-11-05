# 🎧 DhexterBot 2.1 — NOC Cyber Girl (Corpo do Buddy no ESP8266 + Cérebro Hermes)

**DhexterBot 2.1** é um firmware em C++ para o hardware **GeekMagic SmallTV** (`ESP-12F / ESP8266`, 4 MB Flash, tela IPS `1.54" 240x240 ST7789`).

Segue a arquitetura **ESP8266 = Corpo do Buddy | Hermes = Cérebro**:
- **Zero Chaves de IA no Firmware:** nenhuma chave NVIDIA, OpenAI ou Gemini fica gravada no ESP8266.
- **Somente o Hermes como Cérebro:** o ESP8266 conversa exclusivamente com a API protegida do Hermes (`scripts/hermes_buddy_api.py` na porta `9119` no CT100 ou em qualquer host Hermes).
- **Autenticação Mútua (`X-Buddy-Token`):** proteção bidirecional entre o Pet (`ESP8266`) e o Hermes (`CT100`).
- **Memória e Contexto no Hermes:** o serviço `hermes_buddy_api.py` mantém histórico persistente (`~/.hermes/dhexterbot_memory.json`) e contexto de conversas/eventos.
- **Corpo Autônomo no ESP8266:** o ESP8266 cuida de todas as animações a 30 FPS, expressões anime safira, lip-sync, cabelo chanel azul com reflexos ciano, fones com anel LED neon e frases offline caso o Hermes esteja indisponível.
- **Diálogo Paginado com Português:** a janela mantém letras grandes em blocos de **3 linhas de 18 caracteres**, alterna automaticamente mensagens longas sem descartar o restante e desenha acentos e cedilha na fonte compacta do display.
- **Personalidade, Frequência e Horários Ativos Configuráveis:** ajustáveis diretamente na aba **🧠 Cérebro Hermes & Personalidade** do painel web.

---

## 🧠 Como Rodar a Ponte Real Protegida no CT100 (`porta 9119`)

Copie [`scripts/hermes_buddy_api.py`](scripts/hermes_buddy_api.py) para o seu servidor Hermes (`CT100 / 172.1.0.32`) e inicie com um token compartilhado:

```bash
# No CT100 (lê automaticamente as chaves de IA do seu ~/.hermes/.env):
python3 scripts/hermes_buddy_api.py --port 9119 --token "SEU_TOKEN_SECRETO" --pet-ip 10.0.15.235
```

Depois, abra `http://10.0.15.235` → aba **🧠 Cérebro Hermes & Personalidade** e configure:
- **URL da API Protegida do Hermes:** `http://172.1.0.32:9119`
- **Token de Autenticação (`X-Buddy-Token`):** `SEU_TOKEN_SECRETO`
- **Personalidade, Frequência (min) e Horário Ativo (ex: 8h às 23h)**

---

## 📡 Hotspot de Emergência & Captive Portal (`SETUP MODE` — Zero Risco de Brick)

Se a SmallTV for ligada em um local onde nenhuma das redes Wi-Fi salvas (até 5 redes simultâneas) esteja disponível:
1. Ela **nunca bricka**: após 8 segundos sem encontrar as redes salvas, entra automaticamente em **`SETUP MODE`**.
2. A tela da SmallTV exibe em letras grandes:
   - **Wi-Fi para conectar (`Join WiFi:`):** `DhexterBot-Setup` (rede aberta)
   - **Endereço de configuração (`Then open:`):** `http://192.168.4.1`
3. Ao conectar pelo celular ou notebook no Wi-Fi `DhexterBot-Setup`, o **Captive Portal** abre o painel de configuração automaticamente.

---

## 🛠️ Compilação & Atualização OTA (PlatformIO)

```bash
# Regenere src/webui.h e compile o binário para ESP8266
python tools/gzip_webui.py
python -m platformio run -e smalltv

# Enviar via Wi-Fi OTA (substitua pelo IP exibido no painel do DhexterBot)
curl.exe -X POST "http://<IP-DO-PET>/update" -F "firmware=@.pio/build/smalltv/firmware.bin"
```
