#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <HijelHID_BLEKeyboard.h>

// TFT
#define TFT_RST 3
#define TFT_CS 7
#define TFT_DC 2
#define TFT_MOSI 6
#define TFT_SCLK 4

// MPU6050
#define SDA_PIN 1
#define SCL_PIN 5
#define MPU_ADDR 0x68

// Rotary encoder
#define ENC_CLK 0
#define ENC_DT 10
#define ENC_SW 20

Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);
WebServer server(80);
HijelHID_BLEKeyboard keyboard("NEXUS KB");

// Saved Wi-Fi networks
const char *savedSSIDs[] = {
  "wifi1",
  "wifi2"
};

const char *savedPasswords[] = {
  "hellohii123",
  "getyourownpass"
};

const int savedNetworkCount = 2;

// Screens
enum Screen {
  MENU,
  MOTION,
  SYSTEM,
  ENCODER,
  WIFI_MANAGER,
  WIFI_STATUS_SCREEN,
  WEB_INFO,
  MAC_MEDIA
};

Screen currentScreen = MENU;

// Wi-Fi state
enum WifiState {
  WIFI_IDLE,
  WIFI_CONNECTING,
  WIFI_CONNECTED,
  WIFI_FAILED,
  WIFI_CANCELLED
};

WifiState wifiState = WIFI_IDLE;

int availableNetworks[2];
int availableRSSI[2];
int availableNetworkCount = 0;

int selectedWifiItem = 0;
unsigned long wifiStartTime = 0;
unsigned long lastWifiUpdate = 0;

// Main menu
const char *menuItems[] = {
  "Motion Sensor",
  "System Info",
  "Encoder Test",
  "Wi-Fi Connect",
  "Web Dashboard",
  "Mac Media"
};

const int menuCount = 6;

int selected = 0;
int encoderCount = 0;
int lastCLK;

bool lastButton = HIGH;
bool buttonState = HIGH;
unsigned long lastButtonTime = 0;
unsigned long lastUpdate = 0;

// Bluetooth media
unsigned long mediaButtonDownAt = 0;
bool lastBluetoothState = false;
const char *mediaMessage = "Ready";

// MPU6050
uint8_t chipID = 0xFF;

// Web dashboard
const char dashboardHtml[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>NEXUS Dashboard</title>
<style>
*{box-sizing:border-box}
body{
  margin:0;padding:20px;background:#090d16;color:#e5e7eb;
  font-family:Arial,Helvetica,sans-serif
}
main{max-width:850px;margin:auto}
h1{margin:0;font-size:28px;color:#67e8f9}
.subtitle{color:#94a3b8;margin:8px 0 24px}
.grid{
  display:grid;
  grid-template-columns:repeat(auto-fit,minmax(220px,1fr));
  gap:14px
}
.card{
  background:#131b2a;border:1px solid #263449;
  border-radius:14px;padding:18px
}
h2{font-size:15px;color:#93c5fd;margin:0 0 16px}
.label{font-size:12px;color:#94a3b8}
.value{
  font-size:20px;font-weight:600;overflow-wrap:anywhere;
  margin:6px 0 15px
}
.axes{
  display:grid;grid-template-columns:1fr 1fr;gap:14px
}
.axis{font-size:14px;margin:10px 0}
.axis span{color:#67e8f9;font-weight:bold}
.status{color:#fbbf24}
.footer{font-size:12px;color:#64748b;margin-top:20px}
</style>
</head>
<body>
<main>
  <h1>NEXUS</h1>
  <div class="subtitle">ESP32-C3 Live Device Dashboard</div>

  <div class="grid">
    <section class="card">
      <h2>WI-FI CONNECTION</h2>
      <div class="label">Status</div>
      <div class="value status" id="status">Checking...</div>
      <div class="label">Network</div>
      <div class="value" id="ssid">--</div>
      <div class="label">IP Address</div>
      <div class="value" id="ip">--</div>
      <div class="label">Signal Strength</div>
      <div class="value" id="rssi">--</div>
    </section>

    <section class="card">
      <h2>ACCELEROMETER</h2>
      <div class="axes">
        <div class="axis">X: <span id="ax">--</span> g</div>
        <div class="axis">Y: <span id="ay">--</span> g</div>
        <div class="axis">Z: <span id="az">--</span> g</div>
      </div>
    </section>

    <section class="card">
      <h2>GYROSCOPE</h2>
      <div class="axes">
        <div class="axis">X: <span id="gx">--</span> deg/s</div>
        <div class="axis">Y: <span id="gy">--</span> deg/s</div>
        <div class="axis">Z: <span id="gz">--</span> deg/s</div>
      </div>
    </section>
  </div>

  <div class="footer">
    Refreshes every 500 ms · NEXUS local dashboard
  </div>
</main>

<script>
async function updateDashboard(){
  try{
    const response = await fetch('/api/data',{cache:'no-store'});
    if(!response.ok) throw new Error('Request failed');

    const d = await response.json();

    document.getElementById('status').textContent =
      d.connected ? 'Connected' : 'Disconnected';

    document.getElementById('ssid').textContent =
      d.connected ? d.ssid : '--';

    document.getElementById('ip').textContent =
      d.connected ? d.ip : '--';

    document.getElementById('rssi').textContent =
      d.connected ? d.rssi + ' dBm' : '--';

    ['ax','ay','az','gx','gy','gz'].forEach(key=>{
      document.getElementById(key).textContent =
        d.sensorOk && d[key] !== null ? d[key] : '--';
    });
  }catch(e){
    document.getElementById('status').textContent =
      'Device unavailable';
  }
}

updateDashboard();
setInterval(updateDashboard,500);
</script>
</body>
</html>
)rawliteral";

// MPU6050 functions
bool writeReg(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool readRegs(uint8_t reg, uint8_t *data, uint8_t len) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);

  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  if (Wire.requestFrom((uint8_t)MPU_ADDR, len) != len) {
    return false;
  }

  for (uint8_t i = 0; i < len; i++) {
    data[i] = Wire.read();
  }

  return true;
}

int16_t read16(uint8_t *data, uint8_t index) {
  return (int16_t)(((uint16_t)data[index] << 8) | data[index + 1]);
}

// Common UI
void header(const char *title) {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(5, 5);
  tft.print("NEXUS / ");
  tft.println(title);
  tft.drawFastHLine(0, 18, 160, ST77XX_CYAN);
}

void showValue(int x, int y, const char *label, float value) {
  tft.setCursor(x, y);
  tft.print(label);
  tft.print(value, 2);
}

// Main menu
void drawMenu() {
  header("MENU");

  for (int i = 0; i < menuCount; i++) {
    int y = 23 + i * 15;

    if (i == selected) {
      tft.fillRoundRect(3, y - 1, 154, 14, 3, ST77XX_BLUE);
      tft.setTextColor(ST77XX_WHITE);
      tft.setCursor(8, y + 2);
      tft.print("> ");
    } else {
      tft.setTextColor(ST77XX_WHITE);
      tft.setCursor(8, y + 2);
      tft.print("  ");
    }

    tft.println(menuItems[i]);
  }

  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(5, 116);
  tft.println("Turn: Move Press: Open");
}

// System information
void drawSystem() {
  header("SYSTEM INFO");

  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(5, 32);
  tft.println("Device: NEXUS");

  tft.setCursor(5, 49);
  tft.println("Board: ESP32-C3");

  tft.setCursor(5, 66);
  tft.print("MPU address: 0x");
  tft.println(MPU_ADDR, HEX);

  tft.setCursor(5, 83);
  tft.print("Chip ID: 0x");
  tft.println(chipID, HEX);

  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(5, 110);
  tft.println("Press knob: Back");
}

// Encoder test
void drawEncoder() {
  header("ENCODER TEST");

  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(5, 40);
  tft.println("Rotation count:");

  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(2);
  tft.setCursor(5, 57);
  tft.print(encoderCount);

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(5, 95);
  tft.println("Rotate to change");

  tft.setCursor(5, 110);
  tft.println("Press knob: Back");
}

// Motion sensor
void drawMotion() {
  uint8_t data[14];

  if (!readRegs(0x3B, data, 14)) {
    header("MOTION");

    tft.setTextColor(ST77XX_RED);
    tft.setCursor(5, 45);
    tft.println("MPU READ ERROR");

    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(5, 65);
    tft.println("Check connections");
    return;
  }

  int16_t ax = read16(data, 0);
  int16_t ay = read16(data, 2);
  int16_t az = read16(data, 4);

  int16_t gx = read16(data, 8);
  int16_t gy = read16(data, 10);
  int16_t gz = read16(data, 12);

  float accelX = ax / 8192.0f;
  float accelY = ay / 8192.0f;
  float accelZ = az / 8192.0f;

  float gyroX = gx / 65.5f;
  float gyroY = gy / 65.5f;
  float gyroZ = gz / 65.5f;

  tft.fillRect(0, 20, 160, 108, ST77XX_BLACK);

  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(5, 23);
  tft.println("ACCEL (g)");

  tft.setTextColor(ST77XX_WHITE);
  showValue(5, 37, "X:", accelX);
  showValue(85, 37, "Y:", accelY);
  showValue(5, 49, "Z:", accelZ);

  tft.drawFastHLine(0, 64, 160, ST77XX_BLUE);

  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(5, 69);
  tft.println("GYRO (deg/s)");

  tft.setTextColor(ST77XX_WHITE);
  showValue(5, 83, "X:", gyroX);
  showValue(85, 83, "Y:", gyroY);
  showValue(5, 95, "Z:", gyroZ);

  tft.drawFastHLine(0, 109, 160, ST77XX_BLUE);

  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(5, 116);
  tft.println("PRESS KNOB: BACK");
}

// Wi-Fi scanner
void scanSavedNetworks() {
  currentScreen = WIFI_MANAGER;

  header("WIFI SCANNER");

  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(5, 48);
  tft.println("Scanning networks...");

  WiFi.mode(WIFI_STA);

  int found = WiFi.scanNetworks(false, true);

  availableNetworkCount = 0;

  if (found >= 0) {
    for (int saved = 0; saved < savedNetworkCount; saved++) {
      int strongestRSSI = -127;
      bool networkFound = false;

      for (int i = 0; i < found; i++) {
        if (WiFi.SSID(i) == savedSSIDs[saved]) {
          int signal = WiFi.RSSI(i);

          if (!networkFound || signal > strongestRSSI) {
            strongestRSSI = signal;
            networkFound = true;
          }
        }
      }

      if (networkFound && availableNetworkCount < 2) {
        availableNetworks[availableNetworkCount] = saved;
        availableRSSI[availableNetworkCount] = strongestRSSI;
        availableNetworkCount++;
      }
    }
  }

  WiFi.scanDelete();

  selectedWifiItem = 0;
  drawWiFiManager();
}

// Wi-Fi manager list
void drawWiFiManager() {
  header("WIFI CONNECT");

  int itemCount = availableNetworkCount + 2;

  if (selectedWifiItem >= itemCount) {
    selectedWifiItem = itemCount - 1;
  }

  if (availableNetworkCount == 0) {
    tft.setTextColor(ST77XX_YELLOW);
    tft.setCursor(5, 29);
    tft.println("No saved networks");

    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(5, 41);
    tft.println("currently available");

    for (int i = 0; i < 2; i++) {
      int y = 65 + i * 22;

      if (selectedWifiItem == i) {
        tft.fillRoundRect(3, y - 3, 154, 19, 3, ST77XX_BLUE);
        tft.setTextColor(ST77XX_WHITE);
        tft.setCursor(10, y + 2);
        tft.print("> ");
      } else {
        tft.setTextColor(ST77XX_WHITE);
        tft.setCursor(10, y + 2);
        tft.print("  ");
      }

      tft.println(i == 0 ? "Back" : "Rescan");
    }

    tft.setTextColor(ST77XX_YELLOW);
    tft.setCursor(5, 113);
    tft.println("Turn: Move Press: OK");
    return;
  }

  int visibleRows = 4;
  int firstItem = 0;

  if (selectedWifiItem >= visibleRows) {
    firstItem = selectedWifiItem - visibleRows + 1;
  }

  for (int row = 0; row < visibleRows; row++) {
    int item = firstItem + row;

    if (item >= itemCount) {
      break;
    }

    int y = 25 + row * 20;

    if (item == selectedWifiItem) {
      tft.fillRoundRect(3, y - 3, 154, 19, 3, ST77XX_BLUE);
      tft.setTextColor(ST77XX_WHITE);
      tft.setCursor(7, y + 2);
      tft.print("> ");
    } else {
      tft.setTextColor(ST77XX_WHITE);
      tft.setCursor(7, y + 2);
      tft.print("  ");
    }

    if (item < availableNetworkCount) {
      int saved = availableNetworks[item];

      tft.print(savedSSIDs[saved]);
      tft.print(" ");
      tft.print(availableRSSI[item]);
      tft.print("dBm");
    } else if (item == availableNetworkCount) {
      tft.print("Back");
    } else {
      tft.print("Rescan");
    }
  }

  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(5, 111);
  tft.println("Turn: Move Press: OK");
}

// Wi-Fi connection status
void drawWiFiStatus() {
  header("WIFI STATUS");

  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(5, 29);

  if (selectedWifiItem < availableNetworkCount) {
    int saved = availableNetworks[selectedWifiItem];
    tft.println(savedSSIDs[saved]);
  }

  if (wifiState == WIFI_CONNECTING) {
    tft.setTextColor(ST77XX_YELLOW);
    tft.setCursor(5, 51);
    tft.println("Connecting...");

    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(5, 70);
    tft.println("Please wait");

    tft.setTextColor(ST77XX_GREEN);
    tft.setCursor(5, 110);
    tft.println("Press: Cancel");
  }

  else if (wifiState == WIFI_CONNECTED) {
    tft.setTextColor(ST77XX_GREEN);
    tft.setCursor(5, 49);
    tft.println("Connected!");

    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(5, 68);
    tft.print("IP: ");
    tft.println(WiFi.localIP());

    tft.setCursor(5, 85);
    tft.print("Signal: ");
    tft.print(WiFi.RSSI());
    tft.println(" dBm");

    tft.setTextColor(ST77XX_YELLOW);
    tft.setCursor(5, 110);
    tft.println("Press: Back");
  }

  else if (wifiState == WIFI_FAILED) {
    tft.setTextColor(ST77XX_RED);
    tft.setCursor(5, 52);
    tft.println("Connection failed");

    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(5, 72);
    tft.println("Check password");

    tft.setTextColor(ST77XX_YELLOW);
    tft.setCursor(5, 110);
    tft.println("Press: Back");
  }

  else if (wifiState == WIFI_CANCELLED) {
    tft.setTextColor(ST77XX_YELLOW);
    tft.setCursor(5, 52);
    tft.println("Cancelled");

    tft.setCursor(5, 110);
    tft.println("Press: Back");
  }
}

// Start connecting to selected network
void connectToSelectedNetwork() {
  if (selectedWifiItem >= availableNetworkCount) {
    return;
  }

  int saved = availableNetworks[selectedWifiItem];

  WiFi.disconnect(false, false);
  WiFi.begin(savedSSIDs[saved], savedPasswords[saved]);

  wifiState = WIFI_CONNECTING;
  wifiStartTime = millis();
  lastWifiUpdate = 0;

  currentScreen = WIFI_STATUS_SCREEN;
  drawWiFiStatus();
}

// Web dashboard information
void drawWebInfo() {
  header("WEB DASHBOARD");

  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(5, 32);

  if (WiFi.status() == WL_CONNECTED) {
    tft.println("WiFi: Connected");

    tft.setCursor(5, 49);
    tft.println("Open on your Mac:");

    tft.setTextColor(ST77XX_GREEN);
    tft.setCursor(5, 65);
    tft.println(WiFi.localIP());

    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(5, 84);
    tft.println("Same WiFi required");
  } else {
    tft.setTextColor(ST77XX_YELLOW);
    tft.println("WiFi not connected");

    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(5, 49);
    tft.println("Connect to WiFi first");

    tft.setCursor(5, 65);
    tft.println("Then return here");
  }

  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(5, 110);
  tft.println("Press knob: Back");
}

// Bluetooth media screen
void drawMacMedia() {
  header("MAC MEDIA");

  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(5, 30);
  tft.println("Bluetooth HID");

  tft.setCursor(5, 44);

  if (keyboard.isConnected()) {
    tft.setTextColor(ST77XX_GREEN);
    tft.println("Mac: Connected");
  } else {
    tft.setTextColor(ST77XX_RED);
    tft.println("Mac: Disconnected");
  }

  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(5, 62);
  tft.println("Rotate: Volume");

  tft.setCursor(5, 75);
  tft.println("Short press: Play");

  tft.setCursor(5, 88);
  tft.println("Hold: Back to menu");

  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(5, 108);
  tft.println(mediaMessage);
}

// Serve dashboard page
void handleRoot() {
  server.send_P(200, "text/html", dashboardHtml);
}

// Serve live Wi-Fi and MPU6050 data
void handleApiData() {
  bool connected = WiFi.status() == WL_CONNECTED;

  String json;
  json.reserve(320);

  json = "{";

  json += "\"connected\":";
  json += connected ? "true" : "false";

  json += ",\"ssid\":\"";
  if (connected) {
    json += WiFi.SSID();
  }
  json += "\"";

  json += ",\"ip\":\"";
  if (connected) {
    json += WiFi.localIP().toString();
  }
  json += "\"";

  json += ",\"rssi\":";
  json += connected ? String(WiFi.RSSI()) : "0";

  uint8_t data[14];
  bool sensorOk = readRegs(0x3B, data, 14);

  json += ",\"sensorOk\":";
  json += sensorOk ? "true" : "false";

  if (sensorOk) {
    int16_t ax = read16(data, 0);
    int16_t ay = read16(data, 2);
    int16_t az = read16(data, 4);

    int16_t gx = read16(data, 8);
    int16_t gy = read16(data, 10);
    int16_t gz = read16(data, 12);

    json += ",\"ax\":";
    json += String(ax / 8192.0f, 3);

    json += ",\"ay\":";
    json += String(ay / 8192.0f, 3);

    json += ",\"az\":";
    json += String(az / 8192.0f, 3);

    json += ",\"gx\":";
    json += String(gx / 65.5f, 2);

    json += ",\"gy\":";
    json += String(gy / 65.5f, 2);

    json += ",\"gz\":";
    json += String(gz / 65.5f, 2);
  } else {
    json += ",\"ax\":null";
    json += ",\"ay\":null";
    json += ",\"az\":null";
    json += ",\"gx\":null";
    json += ",\"gy\":null";
    json += ",\"gz\":null";
  }

  json += "}";

  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

// Open selected dashboard item
void enterSelected() {
  switch (selected) {
    case 0:
      currentScreen = MOTION;
      header("MOTION");
      break;

    case 1:
      currentScreen = SYSTEM;
      drawSystem();
      break;

    case 2:
      currentScreen = ENCODER;
      drawEncoder();
      break;

    case 3:
      scanSavedNetworks();
      break;

    case 4:
      currentScreen = WEB_INFO;
      drawWebInfo();
      break;

    case 5:
      currentScreen = MAC_MEDIA;
      mediaMessage = "Ready";
      drawMacMedia();
      break;
  }
}

// Encoder handling
void handleEncoder() {
  int clk = digitalRead(ENC_CLK);

  if (clk != lastCLK && clk == LOW) {
    bool clockwise = digitalRead(ENC_DT) != clk;

    if (clockwise) {
      encoderCount++;
    } else {
      encoderCount--;
    }

    if (currentScreen == MENU) {
      if (clockwise) {
        selected++;
        if (selected >= menuCount) selected = 0;
      } else {
        selected--;
        if (selected < 0) selected = menuCount - 1;
      }

      drawMenu();
    }

    else if (currentScreen == ENCODER) {
      drawEncoder();
    }

    else if (currentScreen == WIFI_MANAGER) {
      int itemCount = availableNetworkCount + 2;

      if (clockwise) {
        selectedWifiItem++;
        if (selectedWifiItem >= itemCount) {
          selectedWifiItem = 0;
        }
      } else {
        selectedWifiItem--;
        if (selectedWifiItem < 0) {
          selectedWifiItem = itemCount - 1;
        }
      }

      drawWiFiManager();
    }

    else if (currentScreen == MAC_MEDIA) {
      if (keyboard.isConnected()) {
        if (clockwise) {
          keyboard.tap(MEDIA_VOLUME_UP);
          mediaMessage = "Volume Up sent";
        } else {
          keyboard.tap(MEDIA_VOLUME_DOWN);
          mediaMessage = "Volume Down sent";
        }
      } else {
        mediaMessage = "Connect to Mac";
      }

      drawMacMedia();
    }
  }

  lastCLK = clk;

  // Debounced button handling
  bool reading = digitalRead(ENC_SW);

  if (reading != lastButton) {
    lastButtonTime = millis();
  }

  if (millis() - lastButtonTime > 35) {
    if (reading != buttonState) {
      buttonState = reading;

      // Button pressed
      if (buttonState == LOW) {
        if (currentScreen == MAC_MEDIA) {
          mediaButtonDownAt = millis();
        }

        else if (currentScreen == MENU) {
          enterSelected();
        }

        else if (currentScreen == WIFI_MANAGER) {
          if (selectedWifiItem < availableNetworkCount) {
            connectToSelectedNetwork();
          }

          else if (selectedWifiItem == availableNetworkCount) {
            currentScreen = MENU;
            drawMenu();
          }

          else {
            scanSavedNetworks();
          }
        }

        else if (currentScreen == WIFI_STATUS_SCREEN) {
          if (wifiState == WIFI_CONNECTING) {
            WiFi.disconnect();
            wifiState = WIFI_CANCELLED;
            drawWiFiStatus();
          } else {
            currentScreen = WIFI_MANAGER;
            drawWiFiManager();
          }
        }

        else {
          currentScreen = MENU;
          drawMenu();
        }
      }

      // Button released
      else if (currentScreen == MAC_MEDIA &&
               mediaButtonDownAt != 0) {
        unsigned long pressDuration =
            millis() - mediaButtonDownAt;

        mediaButtonDownAt = 0;

        if (pressDuration >= 700) {
          currentScreen = MENU;
          drawMenu();
        }

        else if (keyboard.isConnected()) {
          keyboard.tap(MEDIA_PLAY_PAUSE);
          mediaMessage = "Play/Pause sent";
          drawMacMedia();
        }

        else {
          mediaMessage = "Connect to Mac";
          drawMacMedia();
        }
      }
    }
  }

  lastButton = reading;
}

// Setup
void setup() {
  pinMode(ENC_CLK, INPUT_PULLUP);
  pinMode(ENC_DT, INPUT_PULLUP);
  pinMode(ENC_SW, INPUT_PULLUP);

  lastCLK = digitalRead(ENC_CLK);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);

  // Initialize MPU6050
  chipID = 0xFF;
  readRegs(0x75, &chipID, 1);

  writeReg(0x6B, 0x00);
  writeReg(0x1C, 0x08);
  writeReg(0x1B, 0x08);
  writeReg(0x1A, 0x03);

  // Start Bluetooth HID
  keyboard.begin();

  // Original Wi-Fi setup
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(false);
  WiFi.persistent(false);
  WiFi.disconnect();

  // Web dashboard routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/data", HTTP_GET, handleApiData);
  server.begin();

  delay(100);

  drawMenu();
}

// Main loop
void loop() {
  handleEncoder();

  // Handle browser requests
  server.handleClient();

  // Live motion screen
  if (currentScreen == MOTION &&
      millis() - lastUpdate >= 150) {
    lastUpdate = millis();
    drawMotion();
  }

  // Wi-Fi connection status updates
  if (currentScreen == WIFI_STATUS_SCREEN &&
      wifiState == WIFI_CONNECTING &&
      millis() - lastWifiUpdate >= 250) {

    lastWifiUpdate = millis();

    if (WiFi.status() == WL_CONNECTED) {
      wifiState = WIFI_CONNECTED;
      drawWiFiStatus();
    }

    else if (millis() - wifiStartTime >= 20000) {
      WiFi.disconnect();
      wifiState = WIFI_FAILED;
      drawWiFiStatus();
    }
  }

  // Update Bluetooth status on the media screen
  bool bluetoothConnected = keyboard.isConnected();

  if (currentScreen == MAC_MEDIA &&
      bluetoothConnected != lastBluetoothState) {
    drawMacMedia();
  }

  lastBluetoothState = bluetoothConnected;
}
