# 👁️ BlinkGuard Wireless v2.0
### ESP8266 Eye Blink Rate Monitor over WiFi (WebSocket)

No USB. No Serial. Connect from any device on the same WiFi network.

---

## 📡 Architecture

```
IR Sensor (glasses)
      │
   ESP8266  ──── WiFi ────►  Browser Dashboard
  (WebSocket                  (index.html)
    Server :81)
```

The ESP8266 runs a **WebSocket server on port 81**.  
Open `index.html` in any browser, enter the ESP's IP, click Connect.

---

## 🩺 Eye Health Reference

| Blink Rate | Status | Meaning |
|---|---|---|
| **< 12 BPM** | ⚠️ Too Low | Dry eyes, screen fatigue, intense focus |
| **12 – 20 BPM** | ✅ Healthy | Normal blink rate |
| **> 20 BPM** | ⚡ Too High | Eye irritation, allergies, fatigue |

---

## 🔧 Hardware Required

| Component | Notes |
|---|---|
| ESP8266 (NodeMCU / Wemos D1 Mini) | 3.3V logic |
| TCRT5000 IR Proximity Sensor | Reflective IR — detects eyelid |
| Glasses frame | Mount sensor on inner side |
| Jumper wires | 3 wires: VCC, GND, OUT |
| WiFi network | ESP8266 + phone/PC on same network |

---

## 🔌 Wiring

```
TCRT5000 IR Sensor     NodeMCU / Wemos D1 Mini
──────────────────     ───────────────────────
    VCC  ──────────►  3.3V   (NOT 5V!)
    GND  ──────────►  GND
    OUT  ──────────►  D2  (GPIO4)
```

> ⚠️ Use **3.3V** only — ESP8266 pins are NOT 5V tolerant.

### Glasses Mount
```
  [Glasses Frame — Inner Side]
  ┌──────────────────────────┐
  │  👁️  ◄── TCRT5000 here  │
  │   (facing down toward    │
  │    eyelid, ~10–15 mm)    │
  └──────────────────────────┘
```

---

## 📦 Required Arduino Libraries

Install via **Arduino IDE → Library Manager**:

| Library | Author | Search Term |
|---|---|---|
| `ESP8266WiFi` | Built-in | (included with ESP8266 board package) |
| `WebSocketsServer` | Markus Sattler | `WebSockets` |
| `ArduinoJson` | Benoit Blanchon | `ArduinoJson` (install v6.x) |

Also install the **ESP8266 board package**:
- Preferences → Additional Board URLs → add:
  `http://arduino.esp8266.com/stable/package_esp8266com_index.json`
- Board Manager → search `esp8266` → Install

---

## 🚀 Setup Steps

### Step 1 — Configure WiFi credentials

Open `BlinkGuard_ESP8266.ino` and change these two lines:

```cpp
const char* WIFI_SSID     = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
```

### Step 2 — Upload to ESP8266

1. Select board: `Tools → Board → NodeMCU 1.0` (or Wemos D1 Mini)
2. Select port: `Tools → Port → COMx`
3. Click **Upload**
4. Open **Serial Monitor** at `115200` baud
5. Watch for:
   ```
   [OK] WiFi Connected!
   [IP] http://192.168.1.xx
   [WS] WebSocket server started on port 81
   ```
6. **Copy the IP address** — you'll need it for the dashboard

### Step 3 — Open Dashboard

1. Open `index.html` in any browser (Chrome, Firefox, Safari, mobile — all work!)
2. Enter the IP address from the serial monitor into the **ESP IP** field
3. Port: `81` (default, don't change)
4. Click **Connect**
5. The WiFi signal bars and status will turn green
6. Start blinking — data updates live!

---

## 📡 WebSocket Protocol

### ESP8266 → Browser

**On every blink:**
```json
{"type":"blink","bpm":15,"total":42,"health":"GOOD","session":120,"blink":true,"rssi":-62}
```

**Every second (stats update):**
```json
{"type":"stats","bpm":15,"total":42,"health":"GOOD","session":121,"blink":false,"rssi":-63}
```

### Browser → ESP8266

| Command | Action |
|---|---|
| `RESET` | Clears all blink counters on ESP8266 |

---

## 🖥️ Dashboard Features

- **Live BPM dial** — animated arc, color-coded health
- **Health badge** — HEALTHY / TOO LOW / TOO HIGH
- **WiFi signal bars** — shows RSSI strength live
- **Session stats** — total blinks, duration, min/max BPM
- **IR sensor visualizer** — ripple animation on each blink
- **Live waveform** — real-time signal trace
- **Event log** — timestamped blink history
- **Blink interval chart** — last 10 intervals in seconds
- **Demo Mode** — test without hardware
- **Works on mobile** — open `index.html` on your phone on same WiFi

---

## ⚙️ Configurable Parameters

```cpp
#define IR_SENSOR_PIN   D2      // Change if you use a different pin
#define DEBOUNCE_MS     150     // Increase if getting false blinks (try 200)
#define STATS_INTERVAL  1000    // Stats broadcast interval (ms)
#define BLINK_WINDOW    60000   // Rolling BPM window (60 seconds)
```

---

## 🛠️ Troubleshooting

| Problem | Solution |
|---|---|
| "WiFi not connecting" | Check SSID/password, ensure 2.4GHz network |
| "Cannot connect to WebSocket" | Ensure phone/PC is on the same WiFi as ESP8266 |
| Dashboard shows Error | Double-check IP address in dashboard matches Serial Monitor |
| No blinks detected | Adjust sensor distance (10–15mm from eyelid) |
| Too many false blinks | Increase `DEBOUNCE_MS` to 200–300 |
| ESP restarts constantly | Check 3.3V power — do not use 5V on sensor |
| Low RSSI (weak signal) | Move ESP8266 closer to router, or use external antenna |

---

## 🔬 How It Works

```
Eye Closes
    │
    ▼
TCRT5000 IR blocked → OUT pin goes LOW
    │
    ▼
ESP8266 detects falling edge (HIGH→LOW)
    │
    ▼
Debounce filter (150ms) → valid blink registered
    │
    ▼
Timestamp stored in 60-second rolling buffer
    │
    ▼
BPM = count of blinks in last 60 seconds
    │
    ▼
JSON sent over WebSocket to browser
    │
    ▼
Dashboard updates live
```

---

## 📁 Files

```
BlinkGuard-Wireless/
├── index.html                 ← Web dashboard (open in any browser)
├── BlinkGuard_ESP8266.ino     ← ESP8266 Arduino sketch
└── README.md                  ← This file
```

---

*BlinkGuard Wireless v2.0 — ESP8266 + WebSocket + Vanilla JS*
