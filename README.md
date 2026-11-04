# 🎧 DhexterBot 2.1 — NOC Cyber Girl (Super Expressiva) + Cérebro Neural & Hermes Bot

**DhexterBot 2.1** é um firmware autônomo em C++ desenvolvido sob medida para o hardware **GeekMagic SmallTV** (`ESP-12F / ESP8266`, 4 MB Flash, tela IPS `1.54" 240x240 ST7789`).

Transforma a SmallTV na **NOC Cyber Girl — Close-Up Anime (Super Expressiva)**: uma companheira de bancada e guardiã de rede com **Cérebro Neural Autônomo (LLM)**, **Login OAuth PKCE 1-Clique**, **Integração Bidirecional Nativa com o Hermes Agent (`hermes-agent`)** e monitoramento direto do **Zabbix NOC (`10.10.1.26`)**.

---

## ✨ Principais Recursos da v2.1

### 1. Visual "NOC Cyber Girl — Close-Up Anime (Super Expressiva)" em 240×240 (30 FPS)
- **Zero Barras de Fome/Level:** 100% da tela dedicada ao retrato anime super expressivo sobre fundo de estúdio escuro limpo e à **janela de diálogo grande de 3 linhas (`12x16` Bold)**.
- **Personagem Fiel à Arte Original:**
  - **Cabelo Chanel Preto Fixo com Pontas Viradas para Cima:** cabelo preto (`0x0842`), contorno (`0x1A56`) e reflexos brancos de quadrinhos (*comic-noir*) na franja permanecem **sempre com as cores originais fixas** (nunca mudam de cor).
  - **Apenas os Headphones Mudam de Cor:** os fones over-ear têm arco branco e anel LED neon duplo que pulsa e muda de cor dinamicamente conforme o estado/humor ou preset (**Azul Ciano** normal, **Verde Esmeralda** tudo UP, **Roxo Neural** falando via IA/Hermes, **Rosa** carinho e **Vermelho Piscante** em queda de link!).
  - **Olhos Anime Safira em Camadas:** esclera branco-azulada, íris com degradê de 3 tons (azul-marinho → safira real → ciano luminoso), duplo brilho cristalino (*catchlights*), delineado superior com puxadinho (*winged eyeliner*), corações (`♥`), estrelas (`★`) ou anel de dados Persocom/Cyber!
  - **Janela de Diálogo Grande (`232x77 px`, Fonte `12x16` Bold):** exibe falas espontâneas da IA, mensagens do Hermes Bot e alertas do NOC em 3 linhas grandes com quebra de palavra automática.
  - **Lip-Sync em Tempo Real:** os lábios se movem sincronizados enquanto ela fala no balão inferior.

### 2. Cérebro Neural Autônomo + Login OAuth PKCE 1-Clique
- **Pensamentos Espontâneos Contextualizados:** periodicamente (ou com 1 clique na Web UI), o próprio ESP8266 consulta o LLM enviando horário NTP, temperatura externa ao vivo (`Open-Meteo`) e estado dos hosts do NOC (`Zabbix` / `Watchdog`), recebendo a frase e o humor em JSON (`~45 tokens` por chamada).
- **6 Provedores Neurais Suportados Direto no ESP8266 (`BearSSL`):**
  - `0` — **NVIDIA NIM (`nvidia/nemotron-3.5-super-vl-120b-a12b`)**: configurado com `enable_thinking: false` para respostas instantâneas.
  - `1` — **Universal Hermes Agent (`http://` ou `https://`)**: conecta-se a **qualquer instância do Hermes Agent** (Nous Research `hermes-agent`, servidor OpenAI-compatible `/v1/chat/completions` ou endpoint REST/Webhook customizado em qualquer IP/hostname/porta), com autenticação Bearer opcional e fallback inteligente.
  - `2` — **OpenRouter (Com Botão `🔗 Conectar via OAuth 1-Clique` na Web UI)**: fluxo OAuth PKCE completo direto no navegador!
  - `3` — **OpenAI Oficial (`gpt-4o-mini`)**
  - `4` — **Google Gemini (`gemini-2.0-flash` / `gemini-flash-lite-latest`)**
  - `5` — **Groq Cloud (`llama-3.3-70b-versatile`)**

### 3. Protocolo Universal para Qualquer Hermes Agent (`/api/hermes` & `/api/hermes/webhook`)
Qualquer instância do **Hermes Agent** em qualquer servidor, container ou máquina pode descobrir as capacidades da DhexterBot via `GET /api/hermes` e controlar a NOC Cyber Girl via `POST /api/hermes/webhook` ou importando a Skill Universal [`scripts/hermes_dhexterbot_bridge.py`](scripts/hermes_dhexterbot_bridge.py):

```bash
# 1. Descoberta automática de Schema & Status (GET /api/hermes):
python scripts/hermes_dhexterbot_bridge.py --host 10.0.15.235 --status

# 2. Notificar conclusão de tarefa de qualquer Hermes Agent:
python scripts/hermes_dhexterbot_bridge.py --host 10.0.15.235 --event success --msg "Hermes: Tarefa concluida com sucesso!"

# 3. Enviar alerta crítico (fones piscam em vermelho + expressão de alerta):
python scripts/hermes_dhexterbot_bridge.py --host 10.0.15.235 --event alert --msg "Hermes: Anomalia critica detectada!" --alerts 1

# 4. Fazer qualquer Hermes conversar com o Cérebro Neural da SmallTV:
python scripts/hermes_dhexterbot_bridge.py --host 10.0.15.235 --event ask --msg "Como estao os sistemas hoje?"
```

---

## 📡 Hotspot de Emergência & Captive Portal (`SETUP MODE` — Zero Risco de Brick)

Se a SmallTV for ligada em um local onde nenhuma das redes Wi-Fi salvas (até 5 redes simultâneas) esteja disponível:
1. Ela **nunca bricka**: após 8 segundos sem encontrar as redes salvas, entra automaticamente em **`SETUP MODE`**.
2. A tela da SmallTV exibe em letras grandes:
   - **Wi-Fi para conectar (`Join WiFi:`):** `DhexterBot-Setup` (rede aberta)
   - **Endereço de configuração (`Then open:`):** `http://192.168.4.1`
3. Ao conectar pelo celular ou notebook no Wi-Fi `DhexterBot-Setup`, o **Captive Portal** abre o painel de configuração automaticamente (ou em `http://192.168.4.1`), permitindo buscar e salvar a nova rede Wi-Fi.
4. Enquanto estiver em `SETUP MODE`, a cada **90 segundos** ela tenta reconectar silenciosamente às redes salvas — se o roteador principal voltar após uma queda de energia, ela retorna sozinha para a **NOC Cyber Girl**.

---

## 🛠️ Compilação & Atualização OTA (PlatformIO)

```bash
# Compilar o binário para ESP8266 (compacta src/webui.html automaticamente)
python -m platformio run -e smalltv

# Enviar via Wi-Fi OTA
curl.exe -X POST "http://10.0.15.235/update" -F "firmware=@.pio/build/smalltv/firmware.bin"
```
