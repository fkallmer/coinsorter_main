#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <LittleFS.h>
#include <esp_mac.h>
#include <esp_task_wdt.h>
#include <Wire.h>
#include "rgb_lcd.h"
#include <ESP32Servo.h>
#include <ArduinoJson.h>  // Bibliothek: "ArduinoJson" by Benoit Blanchon (v6)

// ── Einstellungen ──
static const char APP_VERSION[] = "v2.3";
int sendIntervalMs = 20;  // 20ms = 50 Hz

// ── Pins ──
static const int PIN_CS    = 5;
static const int PIN_SCK   = 18;
static const int PIN_MOSI  = 23;
static const int PIN_MISO  = 19;
static const int PIN_CLKIN = 33;
const int servoPin         = 12;

// ── LDC1101 Register ──
static const uint8_t REG_RP_SET    = 0x01;
static const uint8_t REG_TC1       = 0x02;
static const uint8_t REG_TC2       = 0x03;
static const uint8_t REG_DIG_CONF  = 0x04;
static const uint8_t REG_START_CFG = 0x0B;
static const uint8_t REG_STATUS    = 0x20;
static const uint8_t REG_RP_LSB    = 0x21;
static const uint8_t REG_L_LSB     = 0x23;
#define LDC_SPI_SETTINGS SPISettings(1000000, MSBFIRST, SPI_MODE0)

// ── LDC1101 Konfiguration ──
// Gemessen: fSENSOR≈0.884 MHz (L_DATA=579 bei fCLKIN=4MHz, RESP_TIME=384)
// Rp_Luft≈8.76 kΩ (RPDATA≈60400). fCLKIN=4MHz ≥ 4×fSENSOR=3.54MHz ✓ (Abschnitt 9.1.12)
// → Modelle (k-NN / Baum) müssen mit neuen L-Werten neu trainiert werden.
// RP_SET 0x35: HIGH_Q=0, RP_MAX=12kΩ (b011), RP_MIN=3kΩ (b101)
//   Rp_Luft≈8.76kΩ → RPMAX=12kΩ≤2×8.76; Rp_Münze est.≈4kΩ → RPMIN=3kΩ<0.8×4kΩ
static const uint8_t LDC_RP_SET   = 0x35;
// TC1 0xD2: C1=6pF (b11), R1=18→187kΩ → R1×C1=1.10µs für fSENSOR-MIN≈0.68MHz
//   Formel (Gl.8): R1×C1 = √2 / (π × 0.6V × fSENSOR-MIN)
static const uint8_t LDC_TC1      = 0xD2;
// TC2 0xFC: C2=24pF (b11), R2=60→68.8kΩ → R2×C2=1.65µs
//   Formel (Gl.9): R2×C2 = 2 × RP_MIN × C_SENSOR (angenommen C_SENSOR≈300pF)
//   Wenn C_SENSOR bekannt: R2×C2 = 2×3kΩ×C_SENSOR neu berechnen
static const uint8_t LDC_TC2      = 0xFC;
// DIG_CONF 0x43: MIN_FREQ=4→Watchdog bei 667kHz, RESP_TIME=b011=384
static const uint8_t LDC_DIG_CONF = 0x43;

// ── Objekte ──
WebServer        server(80);
WebSocketsServer ws(81);
rgb_lcd          lcd;
Servo            meinServo;

// ── Sensor-State ──
int      aktuellerWinkel = 90;
int      neutralWinkel   = 90;
uint16_t lastRp          = 0;
uint16_t lastL           = 0;
char     apSSID[32];
char     apPW[16];

// ── Klassifikation ──
enum SortMode : uint8_t { MODE_MANUAL = 0, MODE_THRESHOLD, MODE_KNN, MODE_TREE };
SortMode sortMode = MODE_MANUAL;
enum ThresholdMode : uint8_t { THR_PEAK = 0, THR_AVG, THR_DELTA };
ThresholdMode thresholdMode = THR_PEAK;
uint16_t thresholdWindowMs = 200;

#define MAX_CLASSES 8
#define MAX_SAMPLES 64
#define MAX_TREE_NODES 31
#define FEATURE_COUNT 6
#define MAX_PEAK_LOG  80

struct ThrClass {
  char     name[16];
  uint16_t rp_lo, rp_hi, l_lo, l_hi;
  int      angle;
};
ThrClass thrClasses[MAX_CLASSES];
uint8_t  thrCount     = 0;
uint16_t airThreshold = 55000;

struct ClassInfo {
  char name[16];
  int  angle;
};
ClassInfo modelClasses[MAX_CLASSES];
uint8_t   modelClassCount = 0;

struct KnnSample {
  char  name[16];
  float x[FEATURE_COUNT];
  int   angle;
};
KnnSample knnSamples[MAX_SAMPLES];
uint8_t  knnCount   = 0;
int      knnK       = 3;
float    modelMean[FEATURE_COUNT] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
float    modelStd[FEATURE_COUNT]  = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};

struct TreeNode {
  int8_t feature;
  float threshold;
  int8_t left;
  int8_t right;
  char label[16];
  int angle;
};
TreeNode treeNodes[MAX_TREE_NODES];
uint8_t  treeCount = 0;

char    lastClass[16]  = "?";
uint8_t classHoldCount = 0;
int     displayAngle   = 0;

enum DashboardTab : uint8_t { TAB_SENSOR = 0, TAB_RECORD, TAB_SORT };
DashboardTab activeTab = TAB_SENSOR;

enum SortActionState : uint8_t { SORT_IDLE = 0, SORT_WAIT, SORT_HOLD };
SortActionState sortActionState = SORT_IDLE;
uint32_t sortActionAtMs = 0;
int sortTargetAngle = 90;
static const uint32_t SORT_WAIT_MS = 2000;
static const uint32_t SORT_HOLD_MS = 1200;

static const uint8_t HOLD_N = 1;  // Peak-basierte Klassifikation: ein stabiler Peak reicht

struct PeakFeatures {
  uint16_t rp_min;
  uint16_t l_min;
  uint16_t delta_rp;
  uint16_t delta_l;
  uint16_t rp_avg;
  uint16_t l_avg;
  uint16_t n;
};
PeakFeatures lastPeak = {0, 0, 0, 0, 0, 0, 0};

struct PeakLogEntry {
  PeakFeatures f;
  char label[16];
};
PeakLogEntry peakLog[MAX_PEAK_LOG];
uint8_t  peakLogCount = 0;
bool     peakLogActive = false;

bool     peakActive = false;
bool     peakClassified = false;
bool     peakWindowDone = false;
uint32_t peakStartMs = 0;
uint16_t peakRpMin = 65535;
uint16_t peakLMin = 65535;
uint32_t peakRpSum = 0;
uint32_t peakLSum = 0;
uint16_t peakCount = 0;
uint16_t airRpRef = 55000;
uint16_t airLRef = 0;
uint8_t  airSamples = 0;

// ── Upload-Fallback ──
const char UPLOAD_PAGE[] PROGMEM = R"(<!DOCTYPE html><html><head><meta charset="utf-8"><style>
body{font-family:monospace;background:#0b0f16;color:#e5e7eb;display:flex;align-items:center;justify-content:center;height:100vh;margin:0}
.box{border:1px solid #243041;border-radius:12px;padding:32px;text-align:center;max-width:360px}
h2{margin:0 0 8px;color:#38bdf8}p{color:#9aa4b2;margin:0 0 20px}
input[type=file]{display:none}
.btn{display:inline-block;padding:10px 20px;border-radius:8px;border:1px solid rgba(56,189,248,.5);background:rgba(56,189,248,.1);color:#38bdf8;cursor:pointer;font:13px monospace}
.btn:hover{background:rgba(56,189,248,.2)}progress{width:100%;margin-top:16px;display:none}
</style></head><body><div class="box">
<h2>COINSORTER // LAB</h2><p>Kein Dashboard gefunden.<br>index.html hochladen:</p>
<label class="btn" for="f">Datei wählen</label>
<input type="file" id="f" accept=".html">
<progress id="p"></progress>
<script>
document.getElementById('f').onchange=function(){
  var fd=new FormData();fd.append('file',this.files[0]);
  var x=new XMLHttpRequest();
  x.upload.onprogress=function(e){var p=document.getElementById('p');p.style.display='block';p.value=e.loaded/e.total;};
  x.onload=function(){location.reload();};
  x.open('POST','/update');x.send(fd);
};
</script></div></body></html>)";


// ── Setup ──
void setup() {
  Serial.begin(115200);

  setupDisplay();
  delay(1200);

  uint8_t mac[6];
  esp_efuse_mac_get_default(mac);
  snprintf(apSSID, sizeof(apSSID), "CoinSorter_%02X%02X", mac[4], mac[5]);

  // 8-stelliges PW deterministisch aus MAC (FNV-1a)
  uint32_t h = 2166136261UL;
  for (uint8_t i = 0; i < 6; i++) { h ^= mac[i]; h *= 16777619UL; }
  snprintf(apPW, sizeof(apPW), "%08lu", (unsigned long)(h % 100000000UL));

  lcd.setCursor(0, 1); lcd.print("LDC...  ");
  ledcAttach(PIN_CLKIN, 4000000, 2);
  ledcWrite(PIN_CLKIN, 2);  // 50% duty bei 2-bit Aufloesung
  pinMode(PIN_CS, OUTPUT);
  digitalWrite(PIN_CS, HIGH);
  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI);
  resetLDC();

  lcd.setCursor(0, 1); lcd.print("FS...   ");
  esp_task_wdt_deinit();
  LittleFS.begin(true);
  loadKnnModel();

  lcd.setCursor(0, 1); lcd.print("WiFi... ");
  WiFi.softAP(apSSID, apPW);

  setupRoutes();
  ws.begin();
  ws.onEvent(onWsEvent);

  lcd.setCursor(0, 1); lcd.print("Servo...");
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  meinServo.setPeriodHertz(50);
  meinServo.attach(servoPin, 500, 2400);
  meinServo.write(aktuellerWinkel);

  showSsidScreen();
}


// ── Hauptschleife ──
void loop() {
  server.handleClient();
  ws.loop();
  processSortAction();

  // LCD alle 500ms
  static uint32_t lastLcdMs = 0;
  static uint8_t  prevCount = 255;
  static bool     dataShown = false;
  if (millis() - lastLcdMs >= 500) {
    lastLcdMs = millis();
    uint8_t count = (uint8_t)WiFi.softAPgetStationNum();
    if (count == 0) {
      if (dataShown || prevCount != 0) { showSsidScreen(); dataShown = false; }
    } else {
      if (!dataShown) { lcd.clear(); lcd.setRGB(0, 255, 80); dataShown = true; }
      showDataScreen(count);
    }
    prevCount = count;
  }

  // Sensor lesen + klassifizieren + senden
  static uint32_t lastSensorMs = 0;
  if (millis() - lastSensorMs >= (uint32_t)sendIntervalMs) {
    lastSensorMs = millis();

    SPI.beginTransaction(LDC_SPI_SETTINGS);
    uint8_t  status = ldcRead8(REG_STATUS);
    uint16_t rp, l;
    ldcReadRpL(&rp, &l);
    SPI.endTransaction();

    // Bit7=NO_SENSOR_OSC: Oszillator ausgefallen (z.B. Objekt direkt auf Sensor)
    static uint8_t errCnt = 0;
    if ((rp == 0 && l == 0) || (status & 0x80)) {
      if (++errCnt > 3) { resetLDC(); errCnt = 0; }
    } else {
      errCnt = 0;
    }

    lastRp = rp;
    lastL  = l;

    updatePeak(rp, l);
    broadcastSensor(rp, l);
  }
}
