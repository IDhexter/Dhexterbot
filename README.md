# 🤖 DhexterBot 2.0 — Companheiro Virtual de Bancada & Guardião NOC

**DhexterBot 2.0** é um sistema operacional / firmware autônomo em C++ desenvolvido especificamente para o hardware **GeekMagic SmallTV** (`ESP-12F / ESP8266`, 4 MB Flash, tela IPS `1.54" 240x240 ST7789`).

Transforma a telinha em um **robô companheiro carismático** (inspirado em *Anki Cozmo*, *Living.AI Emo* e *BMO*) com física elástica a **30 FPS (zero flicker)**, necessidades estilo **Tamagotchi**, **Timer Pomodoro** e **integração direta com sistemas de monitoramento (Zabbix JSON-RPC & Watchdog TCP/IP)**.

---

## ✨ Principais Recursos

### 1. Motor Gráfico Scanline 30 FPS & Carisma Procedural
- **Renderização Scanline sem Flicker:** desenha o rosto completo (`216x140 px`) linha a linha usando apenas `432 bytes` de stack (zero alocação de Heap), mantendo mais de **24 KB de RAM livre** no ESP8266.
- **Física Elástica (*Spring-Damper Physics*):** pupilas e pálpebras se movem com inércia e elasticidade orgânica, com piscadas simples e duplas naturais.
- **Rosto Completo Expressivo:**
  - **12 Humores:** `Auto (IA)`, `Normal`, `Feliz (^_^)`, `Empolgado (★_★)`, `Apaixonado (♥_♥)`, `Cool (Óculos Escuros 😎)`, `Curioso (o_O)`, `Surpreso (O_O)`, `Cyber Scanner (KITT)`, `Bravo / Alerta (>_<)`, `Sonolento` e `Dormindo (zZz)`.
  - **Boca Animada Sincronizada:** sorri, faz expressões e abre/fecha em tempo real enquanto o robô fala no balão de diálogo.
  - **Bochechas Coradas (*Blush*):** bochechas rosa/coral em pixel-art quando feliz, apaixonado ou recebendo carinho.
  - **Acessórios Customizáveis:** Óculos Escuros *Deal With It* 😎, Headset de Operador NOC com microfone 🎧, Antena Robótica luminosa 📡 e Coroa Pixel Dourada 👑.
- **Clima Autônomo Reativo (Open-Meteo):** consulta a temperatura e condição climática local via Wi-Fi sem precisar de PC ou chave de API, desenhando gotas de chuva na tela quando chove na sua cidade.

### 2. Interações Tamagotchi & Pomodoro de Bancada
- **Atributos Vivos:** barras de **⚡ Energia**, **☕ Café**, **❤️ Felicidade** e sistema de **🏆 Nível / XP**.
- **Animações de Ação:**
  - `☕ Dar Café`: desenha uma xícara fumegante na tela enquanto o DhexterBot toma goles e recupera Energia/Café.
  - `👋 Fazer Cafuné`: olhos fecham de alegria com corações flutuando e ganho de XP.
  - `⚡ Recarregar Bateria`: raios neon elétricos nas laterais do rosto.
  - `💻 Modo Hacker`: chuva digital estilo *Matrix* escorrendo ao fundo dos olhos.
  - `🎉 Comemorar`: estrelas e confetes celebrando deploys ou recuperação de links.
- **🍅 Timer Pomodoro Integrado:** contagens de foco (`25 min`, `50 min`, pausa `5 min`) no topo da tela com ativação automática do Headset NOC.

### 3. Três Modos de Visualização na Tela (`viewMode`)
1. **`0` — Rosto Completo Carismático:** Rosto grande + HUD superior + Balão de fala + Barras de vida Tamagotchi.
2. **`1` — Relógio Digital Gigante + DhexterBot:** O robô fica na metade superior espiando o relógio digital grande (`HH:MM:SS`), data e temperatura.
3. **`2` — Matriz Guardião NOC + DhexterBot:** O robô reage na metade superior enquanto a metade inferior exibe em tempo real o status (`UP` com latência em `ms` ou `DOWN!`) dos 4 hosts monitorados e alertas do Zabbix.

### 4. Integração Nativa com Sistemas & NOC
- **Watchdog TCP/IP Autônomo (4 Slots):** o próprio ESP8266 testa portas TCP na LAN/WAN (ex: `Internet 1.1.1.1:53`, `Zabbix 10.10.1.26:80`, Switches, Gateways). Se algum cair, o DhexterBot muda para expressão de alerta (`>_<`) e avisa qual ativo caiu; quando volta, ele comemora!
- **Conector Direto Zabbix API (`api_jsonrpc.php`):** consulta `problem.get` diretamente pela rede Wi-Fi e reage à contagem de incidentes ativos.
- **API REST / Webhook (`/api/pet` e `/api/notify`):** receba comandos e alertas a partir de scripts PowerShell, Zabbix Actions, Grafana ou Home Assistant.

---

## 🌐 Exemplos de Automação via API (`POST /api/pet`)

### Enviar fala e expressão:
```bash
curl -X POST http://<IP_DA_SMALLTV>/api/pet \
  -H "Content-Type: application/json" \
  -d '{"say":"Deploy concluido com sucesso!","mood":"excited","ttl":30}'
```

### Acionar animação Tamagotchi (`coffee`, `pet`, `charge`, `hacker`, `celebrate`):
```bash
curl -X POST http://<IP_DA_SMALLTV>/api/pet \
  -H "Content-Type: application/json" \
  -d '{"action":"coffee"}'
```

### Enviar Alerta de Monitoramento / Zabbix:
```bash
curl -X POST http://<IP_DA_SMALLTV>/api/pet \
  -H "Content-Type: application/json" \
  -d '{"alertSource":"ZABBIX","alertMsg":"Link WAN Vivo Offline!","alertCount":1}'
```

---

## 🛠️ Compilação & Atualização OTA (PlatformIO)

```bash
# 1. Regenerar o painel web compactado (webui.h) se editar src/webui.html
python tools/gzip_webui.py

# 2. Compilar o binário para ESP8266 (GeekMagic SmallTV)
pio run -e smalltv

# 3. Enviar via Wi-Fi OTA
curl -X POST "http://<IP_DA_SMALLTV>/update" -F "firmware=@.pio/build/smalltv/firmware.bin"
```
