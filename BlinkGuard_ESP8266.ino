/*
 * =====================================================
 *  BlinkGuard v3.0
 *  Hardware : ESP8266 (NodeMCU / Wemos D1 Mini)
 *             Standard IR Obstacle Avoidance Module
 *             (Active LOW — OUT goes LOW on detection)
 *  Protocol : WebSocket (port 81) over WiFi
 * =====================================================
 *
 *  HOW THIS SENSOR WORKS:
 *    OUT = HIGH  →  No reflection  →  Eye OPEN
 *    OUT = LOW   →  Reflection     →  Eye CLOSED (blink!)
 *
 *  WIRING:
 *    IR Module OUT  →  D2 (GPIO4)
 *    IR Module VCC  →  3.3V
 *    IR Module GND  →  GND
 *    Onboard LED    →  D4 (GPIO2, active LOW)
 *
 *  SENSOR PLACEMENT:
 *    Mount on glasses inner frame, facing the EYELID
 *    from the SIDE (not straight at the eye).
 *    Distance: 5–15mm from eyelid surface.
 *    Turn the potentiometer on the module to adjust
 *    sensitivity until the module LED turns ON when
 *    you close your eye and OFF when open.
 *
 *  LIBRARIES (install via Library Manager):
 *    - WebSockets  by Markus Sattler
 *    - ArduinoJson by Benoit Blanchon (v6)
 * =====================================================
 */

#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>

// ─── WiFi ─────────────────────────────────────────────
const char* WIFI_SSID     = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// ─── Pins ─────────────────────────────────────────────
#define IR_PIN    D2   // IR sensor OUT pin (active LOW)
#define LED_PIN   D4   // Onboard LED (active LOW on NodeMCU)

// ─── Tuning ───────────────────────────────────────────
#define BLINK_MIN_MS    40     // Eyelid must stay closed at least 40ms
#define BLINK_MAX_MS    500    // Eyelid closed longer = not a blink (squint/eyes shut)
#define DEBOUNCE_MS     250    // Minimum gap between two blinks
#define STATS_EVERY_MS  1000   // Send stats to dashboard every 1 second
#define BPM_WINDOW_MS   60000  // Rolling 60-second BPM window

// ─── State ────────────────────────────────────────────
bool          eyeClosed       = false;
unsigned long eyeClosedAt     = 0;
unsigned long lastBlinkAt     = 0;
unsigned long lastStatsAt     = 0;
unsigned long sessionStart    = 0;

unsigned long blinkLog[60];   // Circular buffer — timestamps of last 60 blinks
int  blinkIdx    = 0;
int  totalBlinks = 0;
int  clients     = 0;

WebSocketsServer ws(81);

// ─── BPM ──────────────────────────────────────────────
int getBPM() {
  unsigned long now = millis();
  int n = 0;
  for (int i = 0; i < 60; i++)
    if (blinkLog[i] && (now - blinkLog[i]) < BPM_WINDOW_MS) n++;
  return n;
}

const char* health(int bpm) {
  if (bpm == 0)      return "NO_DATA";
  if (bpm < 12)      return "LOW";
  if (bpm <= 20)     return "GOOD";
  return "HIGH";
}

// ─── Send JSON ────────────────────────────────────────
void send(bool isBlink) {
  int bpm = getBPM();
  StaticJsonDocument<200> doc;
  doc["type"]    = isBlink ? "blink" : "stats";
  doc["blink"]   = isBlink;
  doc["bpm"]     = bpm;
  doc["total"]   = totalBlinks;
  doc["health"]  = health(bpm);
  doc["session"] = (millis() - sessionStart) / 1000;
  doc["rssi"]    = WiFi.RSSI();
  char buf[200];
  serializeJson(doc, buf);
  ws.broadcastTXT(buf);
}

// ─── WebSocket events ─────────────────────────────────
void onWsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t len) {
  if (type == WStype_CONNECTED) {
    clients++;
    Serial.printf("[WS] Client connected. IP: %s\n",
      ws.remoteIP(num).toString().c_str());
    send(false); // send current stats immediately
  }
  if (type == WStype_DISCONNECTED && clients > 0) clients--;
  if (type == WStype_TEXT) {
    if (strcmp((char*)payload, "RESET") == 0) {
      totalBlinks = 0; blinkIdx = 0;
      memset(blinkLog, 0, sizeof(blinkLog));
      sessionStart = millis();
      Serial.println("[CMD] Session reset");
    }
  }
}

// ─── Setup ────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== BlinkGuard v3.0 ===");

  pinMode(IR_PIN,  INPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // OFF

  memset(blinkLog, 0, sizeof(blinkLog));

  // Connect WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  Serial.printf("Connecting to %s", WIFI_SSID);
  int t = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(400); Serial.print(".");
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    if (++t > 50) { Serial.println("\nFailed. Restarting."); ESP.restart(); }
  }
  digitalWrite(LED_PIN, HIGH);

  Serial.printf("\n[OK] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("[WS] ws://%s:81\n",          WiFi.localIP().toString().c_str());
  Serial.println("[TIP] Adjust potentiometer on IR module until:");
  Serial.println("      - Module LED = OFF  when eye is OPEN");
  Serial.println("      - Module LED = ON   when eye is CLOSED");

  ws.begin();
  ws.onEvent(onWsEvent);
  sessionStart = millis();

  // 3 quick flashes = ready
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, LOW);  delay(80);
    digitalWrite(LED_PIN, HIGH); delay(80);
  }
  Serial.println("[OK] Ready — watching for blinks.");
}

// ─── Loop ─────────────────────────────────────────────
void loop() {
  ws.loop();

  unsigned long now  = millis();
  bool          pinLow = (digitalRead(IR_PIN) == LOW); // LOW = eye closed

  // ── Rising edge: eye just CLOSED ──────────────────
  if (pinLow && !eyeClosed) {
    eyeClosed   = true;
    eyeClosedAt = now;
    digitalWrite(LED_PIN, LOW); // LED on while eye closed
  }

  // ── Falling edge: eye just OPENED ─────────────────
  if (!pinLow && eyeClosed) {
    eyeClosed = false;
    digitalWrite(LED_PIN, HIGH);

    unsigned long duration = now - eyeClosedAt;
    unsigned long gap      = now - lastBlinkAt;

    // Valid blink check
    if (duration >= BLINK_MIN_MS &&
        duration <= BLINK_MAX_MS &&
        gap      >= DEBOUNCE_MS) {

      lastBlinkAt = now;
      totalBlinks++;
      blinkLog[blinkIdx % 60] = now;
      blinkIdx++;

      send(true); // instant blink event to dashboard

      Serial.printf("[BLINK] #%d | closed=%lums | BPM=%d | %s\n",
        totalBlinks, duration, getBPM(), health(getBPM()));

    } else {
      // Log why it was rejected (helpful for tuning)
      if (duration < BLINK_MIN_MS)
        Serial.printf("[skip] too short: %lums\n", duration);
      else if (duration > BLINK_MAX_MS)
        Serial.printf("[skip] too long: %lums\n", duration);
      else
        Serial.printf("[skip] debounce: gap=%lums\n", gap);
    }
  }

  // ── Periodic stats to dashboard ───────────────────
  if (now - lastStatsAt >= STATS_EVERY_MS) {
    lastStatsAt = now;
    if (clients > 0) send(false);
  }
}
