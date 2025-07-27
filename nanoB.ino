#define THIS_IS_A 0                 // 0 → this is board B

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>

/* ===== Wi-Fi credentials  ===== */
const char* WIFI_SSID = "YOUR WIFI NAME";
const char* WIFI_PASS = "YOUR WIFI PASSWORD";

/* ===== HiveMQ Cloud credentials  ===== */
const char* MQTT_BROKER = "YOUR MQTT BROKER LINK";
const int   MQTT_PORT   = 8883; // HiveMQ Cloud uses port 8883 for secure MQTT connections, replace with your own if needed
const char* MQTT_USER   = "USERNAME"; // HiveMQ Cloud username, replace with your own
const char* MQTT_PASS   = "PASSWORD"; // HiveMQ Cloud password, replace with your own

/* ===== Unique IDs (auto-selected) ===== */
#if THIS_IS_A
  #define CLIENT_ID "esp32NanoA"
  const char* TX_TOPIC = "nanos/AtoB";
  const char* RX_TOPIC = "nanos/BtoA";
#else
  #define CLIENT_ID "esp32NanoB"
  const char* TX_TOPIC = "nanos/BtoA";
  const char* RX_TOPIC = "nanos/AtoB";
#endif

/* ===== 3.  LET’S ENCRYPT ROOT CA  ===== */
const char LETS_ENCRYPT_CA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
YOUR LONG STRING
-----END CERTIFICATE-----
)EOF";



/* ---------- hardware ---------- */
constexpr uint8_t LED_PIN = D7; // REPLACE WITH YOUR OWN PIN NUMBER
constexpr uint8_t LED_CNT = 12; // REPLACE WITH YOUR OWN PIN NUMBER
constexpr uint8_t BTN_PIN = D4; // REPLACE WITH YOUR OWN PIN NUMBER



#define MQTT_KEEPALIVE_SEC 60        // every 60 s → HiveMQ never sleeps

WiFiClientSecure net;
PubSubClient     mqtt(net);
Adafruit_NeoPixel ring(LED_CNT, LED_PIN, NEO_GRB + NEO_KHZ800);

/* ------------------------------------------------------------------
 *                           STATE DEFINITIONS
 * ------------------------------------------------------------------ */
enum class PingState : uint8_t { Idle, Waiting, Acked, Timeout };
PingState pingState   = PingState::Idle;

constexpr uint32_t ACK_TIMEOUT_MS = 3000;
uint32_t           ackDeadline    = 0;

/* Blue spinner (while waiting for ACK) */
constexpr uint16_t SPIN_MS = 80;
uint8_t   spinStep      = 0;
uint32_t  nextSpinFrame = 0;

/* Red / Green blink (after ACK or timeout) */
constexpr uint8_t  BLINK_COUNT = 2;
constexpr uint16_t BLINK_MS    = 250;
bool      blinkOn        = false;
uint8_t   blinkToggles   = 0;
uint32_t  nextBlink      = 0;

/* Smooth “receiver glow” (green-yellow **plus purple** pulse) */
constexpr uint32_t RECV_EFFECT_MS   = 8000;   // longer, more sensual
constexpr uint16_t RECV_FRAME_MS    = 40;
bool      recvEffectActive = false;
uint32_t  recvEffectEnd    = 0;
uint32_t  nextRecvFrame    = 0;
uint8_t   recvPhase        = 0;

/* ------------------------------------------------------------------
 *                      HELPER & DEBUG FUNCTIONS
 * ------------------------------------------------------------------ */
void showSolid(uint8_t r, uint8_t g, uint8_t b)
{
  for (uint8_t i = 0; i < LED_CNT; ++i) ring.setPixelColor(i, r, g, b);
  ring.show();
}

/* -------- Wi-Fi event callback (prints disconnect reasons) -------- */
void onWiFiEvent(arduino_event_id_t event, arduino_event_info_t info)
{
  if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
    Serial.printf("\n[WiFi] Disconnected. Reason: %d\n", info.wifi_sta_disconnected.reason);
  }
}

/* ---------- Connect to Wi-Fi with timeout & diagnostics ---------- */
void connectWiFi()
{
  if (WiFi.status() == WL_CONNECTED) return;

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  const uint32_t start = millis();
  Serial.print("[WiFi] Connecting");
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(250);
    Serial.print('.');
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("\n[WiFi] Connected – IP: %s  RSSI: %d dBm\n",
                  WiFi.localIP().toString().c_str(), WiFi.RSSI());
  } else {
    Serial.println("\n[WiFi] *** still not connected – see reason above ***");
  }
}

/* ---------- Connect / stay connected to MQTT ---------- */
void connectMQTT()
{
  mqtt.setKeepAlive(MQTT_KEEPALIVE_SEC);          // <-- NEW
  while (!mqtt.connected()) {
    if (mqtt.connect(CLIENT_ID, MQTT_USER, MQTT_PASS)) {
      mqtt.subscribe(RX_TOPIC);
      mqtt.publish(TX_TOPIC, "hello");
      Serial.println("[MQTT] Connected");
    } else {
      Serial.print("[MQTT] failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" – retrying in 1.5 s");
      delay(1500);
    }
  }
}

/* ---------------- MQTT message-received handler ------------------ */
void onMessage(char* topic, byte* payload, unsigned int len)
{
  if (strcmp(topic, RX_TOPIC) != 0) return;

  char msg[8] = {0};
  len = (len > 7) ? 7 : len;
  memcpy(msg, payload, len);

  if (strcmp(msg, "ack") == 0 && pingState == PingState::Waiting) {
    pingState     = PingState::Acked;
    blinkOn       = false;
    blinkToggles  = BLINK_COUNT * 2;
    nextBlink     = millis();
  }
  else if (strcmp(msg, "ping") == 0) {
    mqtt.publish(TX_TOPIC, "ack");

    recvEffectActive = true;
    recvEffectEnd    = millis() + RECV_EFFECT_MS;
    nextRecvFrame    = millis();
    recvPhase        = 0;
  }
}

/* ------------------------------------------------------------------
 *                                SETUP
 * ------------------------------------------------------------------ */
void setup()
{
  Serial.begin(115200);
  Serial.println("\n=== ESP32-S3 Nano Ping ===");

  pinMode(BTN_PIN, INPUT_PULLUP);

  ring.begin();
  ring.setBrightness(200);
  ring.clear();
  ring.show();

  WiFi.onEvent(onWiFiEvent);   // diagnostic hook
  net.setCACert(LETS_ENCRYPT_CA);

  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setKeepAlive(MQTT_KEEPALIVE_SEC);          // <-- NEW
  mqtt.setCallback(onMessage);
}

/* ------------------------------------------------------------------
 *                                LOOP
 * ------------------------------------------------------------------ */
void loop()
{
  connectWiFi();
  if (WiFi.status() == WL_CONNECTED) connectMQTT();
  mqtt.loop();                               // auto-PINGREQs every 30 s

  const uint32_t now = millis();

  /* ---------- button edge detection ---------- */
  static bool lastBtn = HIGH;
  bool nowBtn = digitalRead(BTN_PIN);

  if (nowBtn == LOW && lastBtn == HIGH && pingState == PingState::Idle) {
    mqtt.publish(TX_TOPIC, "ping");

    pingState     = PingState::Waiting;
    spinStep      = 0;
    nextSpinFrame = now;
    ackDeadline   = now + ACK_TIMEOUT_MS;
  }
  lastBtn = nowBtn;

  /* ---------- blue spinner ---------- */
  if (pingState == PingState::Waiting && now >= nextSpinFrame) {
    ring.clear();
    ring.setPixelColor(spinStep, 0, 0, 180);
    ring.show();

    spinStep      = (spinStep + 1) % LED_CNT;
    nextSpinFrame = now + SPIN_MS;
  }

  /* ---------- ACK timeout ---------- */
  if (pingState == PingState::Waiting && now > ackDeadline) {
    pingState     = PingState::Timeout;
    blinkOn       = false;
    blinkToggles  = BLINK_COUNT * 2;
    nextBlink     = now;
  }

  /* ---------- red / green blink ---------- */
  if ((pingState == PingState::Timeout || pingState == PingState::Acked) &&
      blinkToggles && now >= nextBlink) {

    blinkOn = !blinkOn;
    if (blinkOn) {
      if (pingState == PingState::Timeout) showSolid(150,   0,   0); // red
      else                                 showSolid(  0, 150,   0); // green
    } else {
      ring.clear(); ring.show();
    }

    blinkToggles--;
    nextBlink = now + BLINK_MS;

    if (blinkToggles == 0 && !blinkOn) pingState = PingState::Idle;
  }

  /* ---------- smooth receiver glow (green-yellow + purple) ---------- */
  if (recvEffectActive && pingState == PingState::Idle) {
    if (now >= recvEffectEnd) {
      recvEffectActive = false;
      ring.clear(); ring.show();
    }
    else if (now >= nextRecvFrame) {
      uint8_t phase = recvPhase & 0x3F;      // 0-63

      if (phase < 32) {                      // first half: green → yellow
        uint8_t up    = (phase <= 15) ? phase : (31 - phase);
        uint8_t red   = map(up, 0, 15, 0, 150);
        uint8_t green = 150;
        showSolid(red, green, 0);
      } else {                               // second half: purple pulse
        uint8_t sub   = phase - 32;          // 0-31
        uint8_t up    = (sub <= 15) ? sub : (31 - sub);
        uint8_t blue  = map(up, 0, 15, 0, 150);
        uint8_t red   = 150;
        showSolid(red, 0, blue);
      }

      recvPhase     = (recvPhase + 1) & 0x3F;
      nextRecvFrame = now + RECV_FRAME_MS;
    }
  }
}
