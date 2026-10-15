#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <HijelHID_BLEKeyboard.h>
#include <math.h>

// ================= HARDWARE =================

#define TFT_RST 3
#define TFT_CS 7
#define TFT_DC 2
#define TFT_MOSI 6
#define TFT_SCLK 4

#define SDA_PIN 1
#define SCL_PIN 5
#define MPU_ADDR 0x68

#define ENC_CLK 0
#define ENC_DT 10
#define ENC_SW 20

const int SCREEN_W = 160;
const int SCREEN_H = 128;

Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);
WebServer server(80);
HijelHID_BLEKeyboard keyboard("NEXUS KB");

// ================= WIFI =================

const char *savedSSIDs[] = {
  "SVYASA_STAFF",
  "Bikash"
};

const char *savedPasswords[] = {
  "Svyasa*2026",
  "12345678"
};

const int savedNetworkCount = 2;

int availableNetworks[2];
int availableRSSI[2];
int availableNetworkCount = 0;

int selectedWifiItem = 0;

unsigned long wifiStartTime = 0;
unsigned long lastWifiUpdate = 0;

enum WifiState {
  WIFI_IDLE,
  WIFI_CONNECTING,
  WIFI_CONNECTED,
  WIFI_FAILED,
  WIFI_CANCELLED
};

WifiState wifiState = WIFI_IDLE;

// ================= SCREENS =================

enum Screen {
  MENU,
  SYSTEM,
  ENCODER,
  WIFI_MANAGER,
  WIFI_STATUS_SCREEN,
  WEB_INFO,
  MAC_MEDIA,
  SPACE_MENU,
  SENSOR_INFO,
  CALIBRATE_SENSOR,
  PLAYING,
  PAUSED,
  GAME_OVER,
  SCREENSAVER
};

Screen currentScreen = MENU;

// ================= SCREENSAVER =================

Screen lastScreenBeforeSaver = MENU;
const unsigned long SCREENSAVER_IDLE_MS = 30000;
const unsigned long SCREENSAVER_FRAME_MS = 100;
unsigned long lastActivityTime = 0;
unsigned long lastSaverFrame = 0;

struct SaverStar {
  int x;
  int y;
  uint8_t speed;
};

const int SAVER_STAR_COUNT = 24;
SaverStar saverStars[SAVER_STAR_COUNT];


const char *menuItems[] = {
  "System Info",
  "Encoder Test",
  "Wi-Fi Connect",
  "Web Dashboard",
  "Mac Media",
  "Space Shooter"
};

const int menuCount = 6;
int selected = 0;

const char *spaceMenuItems[] = {
  "Start Game",
  "Calibrate Sensor",
  "Sensor Info",
  "Main Menu"
};

const int spaceMenuCount = 4;
int spaceMenuSelection = 0;

const char *pauseMenuItems[] = {
  "Resume",
  "Calibrate Sensor",
  "Main Menu"
};

int pauseSelection = 0;

// ================= ENCODER STATE =================

int lastCLK = HIGH;
bool lastButton = HIGH;
bool buttonDown = false;
bool longPressHandled = false;

unsigned long buttonDownAt = 0;
unsigned long lastButtonChange = 0;
unsigned long lastEncoderChange = 0;

const unsigned long DEBOUNCE_MS = 30;
const unsigned long LONG_PRESS_MS = 700;

int encoderCount = 0;

// ================= BLUETOOTH =================

unsigned long mediaButtonDownAt = 0;
bool lastBluetoothState = false;

const char *mediaMessage = "Ready";

// ================= MPU6050 =================

uint8_t chipID = 0xFF;

struct SensorData {
  float ax;
  float ay;
  float az;
  float gx;
  float gy;
  float gz;
};

SensorData sensorData = {0, 0, 0, 0, 0, 0};

bool sensorOK = false;

// Game orientation
float rollAngle = 0;
float pitchAngle = 0;

float neutralRoll = 0;
float neutralPitch = 0;

float relativeRoll = 0;
float relativePitch = 0;

float filteredRoll = 0;

bool calibrated = false;
bool startAfterCalibration = false;

// ================= GAME VARIABLES =================

float shipX = 80;
const int shipY = 108;

int score = 0;
int lives = 3;

bool shieldActive = false;

unsigned long shieldUntil = 0;
unsigned long lastShieldTrigger = 0;
unsigned long lastShot = 0;
unsigned long lastEnemySpawn = 0;
unsigned long lastFrame = 0;

unsigned long lastMotionUpdate = 0;

struct Bullet {
  float x;
  float y;
  bool active;
};

struct Enemy {
  float x;
  float y;
  float speed;
  bool active;
};

const int MAX_BULLETS = 5;
const int MAX_ENEMIES = 5;

Bullet bullets[MAX_BULLETS];
Enemy enemies[MAX_ENEMIES];

// ================= SENSOR FUNCTIONS =================

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
  return (int16_t)(
    ((uint16_t)data[index] << 8) |
    data[index + 1]
  );
}

bool readSensorData() {
  uint8_t data[14];

  if (!readRegs(0x3B, data, 14)) {
    sensorOK = false;
    return false;
  }

  int16_t ax = read16(data, 0);
  int16_t ay = read16(data, 2);
  int16_t az = read16(data, 4);

  int16_t gx = read16(data, 8);
  int16_t gy = read16(data, 10);
  int16_t gz = read16(data, 12);

  sensorData.ax = ax / 8192.0f;
  sensorData.ay = ay / 8192.0f;
  sensorData.az = az / 8192.0f;

  sensorData.gx = gx / 65.5f;
  sensorData.gy = gy / 65.5f;
  sensorData.gz = gz / 65.5f;

  rollAngle = atan2f(
    sensorData.ay,
    sensorData.az
  ) * 180.0f / PI;

  pitchAngle = atan2f(
    -sensorData.ax,
    sqrtf(
      sensorData.ay * sensorData.ay +
      sensorData.az * sensorData.az
    )
  ) * 180.0f / PI;

  sensorOK = true;
  return true;
}

float normalizeAngle(float angle) {
  while (angle > 180) angle -= 360;
  while (angle < -180) angle += 360;
  return angle;
}

// ================= COMMON UI =================

void header(const char *title) {
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(5, 5);

  tft.print("NEXUS / ");
  tft.println(title);

  tft.drawFastHLine(
    0, 18, SCREEN_W, ST77XX_BLUE
  );
}

void showValue(
  int x,
  int y,
  const char *label,
  float value
) {
  tft.setCursor(x, y);
  tft.print(label);
  tft.print(value, 2);
}

void drawCentered(
  const char *text,
  int y,
  uint16_t color
) {
  int16_t x1, y1;
  uint16_t w, h;

  tft.setTextSize(1);

  tft.getTextBounds(
    text, 0, y, &x1, &y1, &w, &h
  );

  tft.setTextColor(color);
  tft.setCursor((SCREEN_W - w) / 2, y);
  tft.print(text);
}

// ================= SCREENSAVER =================

bool screensaverAllowed() {
  if (currentScreen == PLAYING ||
      currentScreen == PAUSED ||
      currentScreen == GAME_OVER ||
      currentScreen == CALIBRATE_SENSOR) {
    return false;
  }

  if (currentScreen == WIFI_STATUS_SCREEN &&
      wifiState == WIFI_CONNECTING) {
    return false;
  }

  return true;
}

void initializeSaverStars() {
  for (int i = 0; i < SAVER_STAR_COUNT; i++) {
    saverStars[i].x = random(0, SCREEN_W);
    saverStars[i].y = random(0, SCREEN_H);
    saverStars[i].speed = random(1, 4);
  }
}

void drawScreensaver() {
  tft.fillScreen(ST77XX_BLACK);

  for (int i = 0; i < SAVER_STAR_COUNT; i++) {
    uint16_t color = saverStars[i].speed == 3 ? ST77XX_CYAN : ST77XX_BLUE;
    tft.drawPixel(saverStars[i].x, saverStars[i].y, color);

    saverStars[i].y += saverStars[i].speed;
    if (saverStars[i].y >= SCREEN_H) {
      saverStars[i].y = 0;
      saverStars[i].x = random(0, SCREEN_W);
      saverStars[i].speed = random(1, 4);
    }
  }

  // Compact NEXUS logo panel
  tft.drawRoundRect(25, 37, 110, 42, 7, ST77XX_CYAN);
  tft.drawRoundRect(28, 40, 104, 36, 5, ST77XX_BLUE);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(42, 48);
  tft.print("NEXUS");

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(24, 91);
  tft.print("Turn or press knob");
  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(43, 107);
  tft.print("STANDBY");
}

void redrawCurrentScreen() {
  switch (currentScreen) {
    case MENU: drawMenu(); break;
    case SYSTEM: drawSystem(); break;
    case ENCODER: drawEncoder(); break;
    case WIFI_MANAGER: drawWiFiManager(); break;
    case WIFI_STATUS_SCREEN: drawWiFiStatus(); break;
    case WEB_INFO: drawWebInfo(); break;
    case MAC_MEDIA: drawMacMedia(); break;
    case SPACE_MENU: drawSpaceMenu(); break;
    case SENSOR_INFO: drawSensorInfo(); break;
    case CALIBRATE_SENSOR: drawCalibration(); break;
    case PLAYING: drawGame(); break;
    case PAUSED: drawPauseMenu(); break;
    case GAME_OVER: drawGameOver(); break;
    case SCREENSAVER: break;
  }
}

void wakeFromScreensaver() {
  if (currentScreen != SCREENSAVER) return;
  currentScreen = lastScreenBeforeSaver;
  lastActivityTime = millis();
  redrawCurrentScreen();
}

void updateScreensaver() {
  unsigned long now = millis();

  if (currentScreen != SCREENSAVER) {
    if (screensaverAllowed() && now - lastActivityTime >= SCREENSAVER_IDLE_MS) {
      lastScreenBeforeSaver = currentScreen;
      currentScreen = SCREENSAVER;
      lastSaverFrame = 0;
      drawScreensaver();
      lastSaverFrame = now;
    }
    return;
  }

  if (now - lastSaverFrame >= SCREENSAVER_FRAME_MS) {
    lastSaverFrame = now;
    drawScreensaver();
  }
}

// ================= MAIN MENU =================

void drawMenu() {
  header("MENU");

  for (int i = 0; i < menuCount; i++) {
    int y = 23 + i * 15;

    if (i == selected) {
      tft.fillRoundRect(
        3, y - 1, 154, 14, 3,
        ST77XX_BLUE
      );

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

// ================= SYSTEM INFO =================

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

// ================= ENCODER TEST =================

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

// ================= SENSOR INFO =================

void drawSensorInfo() {
  header("SENSOR INFO");

  if (!readSensorData()) {
    tft.setTextColor(ST77XX_RED);
    tft.setCursor(5, 42);
    tft.println("MPU READ ERROR");

    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(5, 60);
    tft.println("Check connections");

    tft.setCursor(5, 110);
    tft.println("Press knob: Back");
    return;
  }

  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(5, 23);
  tft.println("ACCEL (g)");

  tft.setTextColor(ST77XX_WHITE);
  showValue(5, 36, "X:", sensorData.ax);
  showValue(85, 36, "Y:", sensorData.ay);
  showValue(5, 48, "Z:", sensorData.az);

  tft.drawFastHLine(
    0, 62, SCREEN_W, ST77XX_BLUE
  );

  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(5, 67);
  tft.println("GYRO (deg/s)");

  tft.setTextColor(ST77XX_WHITE);
  showValue(5, 80, "X:", sensorData.gx);
  showValue(85, 80, "Y:", sensorData.gy);
  showValue(5, 92, "Z:", sensorData.gz);

  tft.setTextColor(
    calibrated ? ST77XX_GREEN : ST77XX_RED
  );

  tft.setCursor(80, 104);
  tft.print(calibrated ? "Calibrated" : "Not calibrated");

  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(5, 116);
  tft.println("Press knob: Back");
}

// ================= MOTION SCREEN =================

void drawMotion() {
  if (!readSensorData()) {
    header("MOTION");

    tft.setTextColor(ST77XX_RED);
    tft.setCursor(5, 45);
    tft.println("MPU READ ERROR");

    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(5, 65);
    tft.println("Check connections");
    return;
  }

  tft.fillRect(
    0, 20, SCREEN_W, 108, ST77XX_BLACK
  );

  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(5, 23);
  tft.println("ACCEL (g)");

  tft.setTextColor(ST77XX_WHITE);
  showValue(5, 37, "X:", sensorData.ax);
  showValue(85, 37, "Y:", sensorData.ay);
  showValue(5, 49, "Z:", sensorData.az);

  tft.drawFastHLine(
    0, 64, SCREEN_W, ST77XX_BLUE
  );

  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(5, 69);
  tft.println("GYRO (deg/s)");

  tft.setTextColor(ST77XX_WHITE);
  showValue(5, 83, "X:", sensorData.gx);
  showValue(85, 83, "Y:", sensorData.gy);
  showValue(5, 95, "Z:", sensorData.gz);

  tft.drawFastHLine(
    0, 109, SCREEN_W, ST77XX_BLUE
  );

  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(5, 116);
  tft.println("PRESS KNOB: BACK");
}

// ================= SPACE SHOOTER MENU =================

void drawSpaceMenu() {
  header("SPACE SHOOTER");

  drawCentered(
    "GYRO ARCADE",
    25,
    ST77XX_YELLOW
  );

  for (int i = 0; i < spaceMenuCount; i++) {
    int y = 42 + i * 18;

    if (i == spaceMenuSelection) {
      tft.fillRoundRect(
        5, y - 2, 150, 17, 3,
        ST77XX_BLUE
      );

      tft.setTextColor(ST77XX_WHITE);
      tft.setCursor(10, y + 2);
      tft.print("> ");
    } else {
      tft.setTextColor(ST77XX_WHITE);
      tft.setCursor(10, y + 2);
      tft.print("  ");
    }

    tft.println(spaceMenuItems[i]);
  }

  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(5, 116);
  tft.println("Turn: Move Press: OK");
}

// ================= CALIBRATION =================

void drawCalibration() {
  header("CALIBRATION");

  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(5, 29);
  tft.println("Place device in");

  tft.setCursor(5, 41);
  tft.println("your desired");

  tft.setCursor(5, 53);
  tft.println("neutral position.");

  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(5, 73);
  tft.println("Keep it still!");

  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(5, 96);
  tft.println("Press knob to set");

  tft.setCursor(5, 108);
  tft.println("this position zero");
}

void performCalibration() {
  header("CALIBRATING");

  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(5, 38);
  tft.println("Keep device still");

  tft.setCursor(5, 52);
  tft.println("Reading sensor...");

  float rollSum = 0;
  float pitchSum = 0;

  const int samples = 50;
  int validSamples = 0;

  delay(150);

  for (int i = 0; i < samples; i++) {
    if (readSensorData()) {
      rollSum += rollAngle;
      pitchSum += pitchAngle;
      validSamples++;
    }

    delay(15);
  }

  if (validSamples < 40) {
    calibrated = false;

    header("SENSOR ERROR");

    tft.setTextColor(ST77XX_RED);
    tft.setCursor(5, 40);
    tft.println("MPU6050 not found");

    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(5, 62);
    tft.println("Check wiring");

    delay(1200);

    currentScreen = CALIBRATE_SENSOR;
    drawCalibration();
    return;
  }

  neutralRoll = rollSum / validSamples;
  neutralPitch = pitchSum / validSamples;

  filteredRoll = 0;
  relativeRoll = 0;
  relativePitch = 0;

  calibrated = true;

  if (startAfterCalibration) {
    startAfterCalibration = false;
    startGame();
  } else {
    currentScreen = SPACE_MENU;
    drawSpaceMenu();
  }
}

// ================= WIFI SCANNER =================

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
        tft.fillRoundRect(
          3, y - 3, 154, 19, 3,
          ST77XX_BLUE
        );

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

    if (item >= itemCount) break;

    int y = 25 + row * 20;

    if (item == selectedWifiItem) {
      tft.fillRoundRect(
        3, y - 3, 154, 19, 3,
        ST77XX_BLUE
      );

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

void connectToSelectedNetwork() {
  if (selectedWifiItem >= availableNetworkCount) return;

  int saved = availableNetworks[selectedWifiItem];

  WiFi.disconnect(false, false);
  WiFi.begin(savedSSIDs[saved], savedPasswords[saved]);

  wifiState = WIFI_CONNECTING;
  wifiStartTime = millis();
  lastWifiUpdate = 0;

  currentScreen = WIFI_STATUS_SCREEN;
  drawWiFiStatus();
}

// ================= WEB DASHBOARD =================

const char dashboardHtml[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>NEXUS Dashboard</title>
<style>
*{box-sizing:border-box}
body{margin:0;padding:20px;background:#090d16;color:#e5e7eb;font-family:Arial,Helvetica,sans-serif}
main{max-width:850px;margin:auto}
h1{margin:0;font-size:28px;color:#67e8f9}
.subtitle{color:#94a3b8;margin:8px 0 24px}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(220px,1fr));gap:14px}
.card{background:#131b2a;border:1px solid #263449;border-radius:14px;padding:18px}
h2{font-size:15px;color:#93c5fd;margin:0 0 16px}
.label{font-size:12px;color:#94a3b8}
.value{font-size:20px;font-weight:600;overflow-wrap:anywhere;margin:6px 0 15px}
.axes{display:grid;grid-template-columns:1fr 1fr;gap:14px}
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
<div class="label">Status</div><div class="value status" id="status">Checking...</div>
<div class="label">Network</div><div class="value" id="ssid">--</div>
<div class="label">IP Address</div><div class="value" id="ip">--</div>
<div class="label">Signal Strength</div><div class="value" id="rssi">--</div>
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
<div class="footer">Refreshes every 500 ms · NEXUS local dashboard</div>
</main>
<script>
async function updateDashboard(){
 try{
  const response=await fetch('/api/data',{cache:'no-store'});
  if(!response.ok)throw new Error('Request failed');
  const d=await response.json();
  document.getElementById('status').textContent=d.connected?'Connected':'Disconnected';
  document.getElementById('ssid').textContent=d.connected?d.ssid:'--';
  document.getElementById('ip').textContent=d.connected?d.ip:'--';
  document.getElementById('rssi').textContent=d.connected?d.rssi+' dBm':'--';
  ['ax','ay','az','gx','gy','gz'].forEach(key=>{
   document.getElementById(key).textContent=d.sensorOk&&d[key]!==null?d[key]:'--';
  });
 }catch(e){
  document.getElementById('status').textContent='Device unavailable';
 }
}
updateDashboard();
setInterval(updateDashboard,500);
</script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send_P(200, "text/html", dashboardHtml);
}

void handleApiData() {
  bool connected = WiFi.status() == WL_CONNECTED;

  String json;
  json.reserve(320);

  json = "{";

  json += "\"connected\":";
  json += connected ? "true" : "false";

  json += ",\"ssid\":\"";
  if (connected) json += WiFi.SSID();
  json += "\"";

  json += ",\"ip\":\"";
  if (connected) json += WiFi.localIP().toString();
  json += "\"";

  json += ",\"rssi\":";
  json += connected ? String(WiFi.RSSI()) : "0";

  bool ok = readSensorData();

  json += ",\"sensorOk\":";
  json += ok ? "true" : "false";

  if (ok) {
    json += ",\"ax\":" + String(sensorData.ax, 3);
    json += ",\"ay\":" + String(sensorData.ay, 3);
    json += ",\"az\":" + String(sensorData.az, 3);
    json += ",\"gx\":" + String(sensorData.gx, 2);
    json += ",\"gy\":" + String(sensorData.gy, 2);
    json += ",\"gz\":" + String(sensorData.gz, 2);
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

// ================= MAC MEDIA =================

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

// ================= GAME INITIALIZATION =================

void startGame() {
  score = 0;
  lives = 3;
  shipX = 80;

  shieldActive = false;
  shieldUntil = 0;

  lastShot = 0;
  lastEnemySpawn = millis();
  lastShieldTrigger = 0;

  for (int i = 0; i < MAX_BULLETS; i++) {
    bullets[i].active = false;
  }

  for (int i = 0; i < MAX_ENEMIES; i++) {
    enemies[i].active = false;
  }

  filteredRoll = 0;

  currentScreen = PLAYING;
  lastFrame = 0;
}

// ================= PLAYER =================

void shoot() {
  if (millis() - lastShot < 220) return;

  lastShot = millis();

  for (int i = 0; i < MAX_BULLETS; i++) {
    if (!bullets[i].active) {
      bullets[i].x = shipX;
      bullets[i].y = shipY - 9;
      bullets[i].active = true;
      break;
    }
  }
}

void drawShip() {
  int x = (int)shipX;

  if (shieldActive) {
    tft.drawCircle(x, shipY, 12, ST77XX_CYAN);
    tft.drawCircle(x, shipY, 14, ST77XX_BLUE);
  }

  tft.fillTriangle(
    x, shipY - 9,
    x - 8, shipY + 7,
    x + 8, shipY + 7,
    ST77XX_CYAN
  );

  tft.fillCircle(x, shipY, 3, ST77XX_WHITE);

  tft.drawPixel(x - 4, shipY + 9, ST77XX_YELLOW);
  tft.drawPixel(x + 4, shipY + 9, ST77XX_YELLOW);
}

// ================= ENEMIES =================

void spawnEnemy() {
  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (!enemies[i].active) {
      enemies[i].x = random(10, SCREEN_W - 10);
      enemies[i].y = 24;

      enemies[i].speed = random(20, 45) / 10.0f;
      enemies[i].speed += score / 150.0f;

      if (enemies[i].speed > 6.0f) {
        enemies[i].speed = 6.0f;
      }

      enemies[i].active = true;
      break;
    }
  }
}

void drawEnemy(float x, float y) {
  int ix = (int)x;
  int iy = (int)y;

  tft.fillRect(ix - 6, iy - 4, 12, 8, ST77XX_RED);
  tft.fillRect(ix - 9, iy - 1, 3, 5, ST77XX_RED);
  tft.fillRect(ix + 6, iy - 1, 3, 5, ST77XX_RED);

  tft.drawPixel(ix - 3, iy - 1, ST77XX_WHITE);
  tft.drawPixel(ix + 3, iy - 1, ST77XX_WHITE);

  tft.drawPixel(ix - 4, iy + 5, ST77XX_YELLOW);
  tft.drawPixel(ix + 4, iy + 5, ST77XX_YELLOW);
}

// ================= GAME LOGIC =================

void updateGame() {
  if (!readSensorData()) return;

  float gyroMagnitude = sqrtf(
    sensorData.gx * sensorData.gx +
    sensorData.gy * sensorData.gy +
    sensorData.gz * sensorData.gz
  );

  if (gyroMagnitude > 250.0f &&
      millis() - lastShieldTrigger > 1800) {
    shieldActive = true;
    shieldUntil = millis() + 1500;
    lastShieldTrigger = millis();
  }

  relativeRoll = normalizeAngle(rollAngle - neutralRoll);
  relativePitch = normalizeAngle(pitchAngle - neutralPitch);

  filteredRoll +=
    (relativeRoll - filteredRoll) * 0.25f;

  float targetX = 80.0f + filteredRoll * 2.0f;

  if (targetX < 9) targetX = 9;
  if (targetX > 151) targetX = 151;

  shipX += (targetX - shipX) * 0.22f;

  if (shieldActive &&
      (long)(millis() - shieldUntil) >= 0) {
    shieldActive = false;
  }

  unsigned long spawnDelay = 850;

  if (score > 50) spawnDelay = 700;
  if (score > 150) spawnDelay = 550;
  if (score > 300) spawnDelay = 400;

  if (millis() - lastEnemySpawn >= spawnDelay) {
    lastEnemySpawn = millis();
    spawnEnemy();
  }

  for (int i = 0; i < MAX_BULLETS; i++) {
    if (!bullets[i].active) continue;

    bullets[i].y -= 5;

    if (bullets[i].y < 20) {
      bullets[i].active = false;
    }
  }

  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (!enemies[i].active) continue;

    enemies[i].y += enemies[i].speed;

    if (enemies[i].y >= SCREEN_H - 4) {
      enemies[i].active = false;
      lives--;

      if (lives <= 0) {
        currentScreen = GAME_OVER;
        return;
      }

      continue;
    }

    float dx = enemies[i].x - shipX;
    float dy = enemies[i].y - shipY;

    if (fabsf(dx) < 11 && fabsf(dy) < 10) {
      enemies[i].active = false;

      if (!shieldActive) {
        lives--;

        if (lives <= 0) {
          currentScreen = GAME_OVER;
          return;
        }
      } else {
        score += 5;
      }
    }

    for (int j = 0; j < MAX_BULLETS; j++) {
      if (!bullets[j].active || !enemies[i].active) {
        continue;
      }

      if (fabsf(bullets[j].x - enemies[i].x) < 9 &&
          fabsf(bullets[j].y - enemies[i].y) < 7) {
        bullets[j].active = false;
        enemies[i].active = false;
        score += 10;
      }
    }
  }
}

// ================= DRAW GAME =================

void drawStars() {
  for (int i = 0; i < 24; i++) {
    int x = (i * 47 + 13) % SCREEN_W;
    int y = 21 + (i * 29) % 104;

    tft.drawPixel(x, y, ST77XX_BLUE);
  }
}

void drawGame() {
  tft.fillScreen(ST77XX_BLACK);

  drawStars();

  tft.fillRect(0, 0, SCREEN_W, 19, ST77XX_BLACK);

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(3, 5);
  tft.print("S:");
  tft.print(score);

  tft.setTextColor(ST77XX_RED);
  tft.setCursor(92, 5);
  tft.print("L:");
  tft.print(lives);

  for (int i = 0; i < MAX_BULLETS; i++) {
    if (bullets[i].active) {
      tft.fillRect(
        (int)bullets[i].x - 1,
        (int)bullets[i].y - 3,
        3, 6,
        ST77XX_YELLOW
      );
    }
  }

  for (int i = 0; i < MAX_ENEMIES; i++) {
    if (enemies[i].active) {
      drawEnemy(enemies[i].x, enemies[i].y);
    }
  }

  drawShip();

  if (!calibrated) {
    tft.setTextColor(ST77XX_RED);
    tft.setCursor(35, 60);
    tft.println("NOT CALIBRATED");
  }
}

// ================= PAUSE MENU =================

void drawPauseMenu() {
  header("PAUSED");

  drawCentered(
    "GAME PAUSED",
    28,
    ST77XX_YELLOW
  );

  for (int i = 0; i < 3; i++) {
    int y = 47 + i * 21;

    if (i == pauseSelection) {
      tft.fillRoundRect(
        8, y - 3, 144, 18, 4,
        ST77XX_BLUE
      );

      tft.setTextColor(ST77XX_WHITE);
      tft.setCursor(17, y + 2);
      tft.print("> ");
    } else {
      tft.setTextColor(ST77XX_WHITE);
      tft.setCursor(17, y + 2);
      tft.print("  ");
    }

    tft.println(pauseMenuItems[i]);
  }

  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(5, 113);
  tft.println("Turn: Move Press: OK");
}

// ================= GAME OVER =================

void drawGameOver() {
  header("GAME OVER");

  drawCentered(
    "MISSION FAILED",
    34,
    ST77XX_RED
  );

  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(38, 57);
  tft.print("Score: ");
  tft.println(score);

  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(5, 82);
  tft.println("Press: Play Again");

  tft.setCursor(5, 101);
  tft.println("Hold: Main Menu");
}

// ================= SCREEN NAVIGATION =================

void enterSelected() {
  switch (selected) {
    case 0:
      currentScreen = SYSTEM;
      drawSystem();
      break;

    case 1:
      currentScreen = ENCODER;
      drawEncoder();
      break;

    case 2:
      scanSavedNetworks();
      break;

    case 3:
      currentScreen = WEB_INFO;
      drawWebInfo();
      break;

    case 4:
      currentScreen = MAC_MEDIA;
      mediaMessage = "Ready";
      drawMacMedia();
      break;

    case 5:
      currentScreen = SPACE_MENU;
      spaceMenuSelection = 0;
      drawSpaceMenu();
      break;
  }
}

void enterSpaceMenuItem() {
  switch (spaceMenuSelection) {
    case 0:
      if (!calibrated) {
        startAfterCalibration = true;
        currentScreen = CALIBRATE_SENSOR;
        drawCalibration();
      } else {
        startGame();
      }
      break;

    case 1:
      startAfterCalibration = false;
      currentScreen = CALIBRATE_SENSOR;
      drawCalibration();
      break;

    case 2:
      currentScreen = SENSOR_INFO;
      drawSensorInfo();
      break;

    case 3:
      currentScreen = MENU;
      drawMenu();
      break;
  }
}

// ================= ENCODER HANDLING =================

void handleEncoder() {
  int clk = digitalRead(ENC_CLK);

  if (clk != lastCLK &&
      clk == LOW &&
      millis() - lastEncoderChange > 2) {

    lastEncoderChange = millis();

    bool clockwise = digitalRead(ENC_DT) != clk;

    encoderCount += clockwise ? 1 : -1;
    lastActivityTime = millis();

    if (currentScreen == SCREENSAVER) {
      wakeFromScreensaver();
    }
    else if (currentScreen == MENU) {
      selected += clockwise ? 1 : -1;

      if (selected >= menuCount) selected = 0;
      if (selected < 0) selected = menuCount - 1;

      drawMenu();
    }

    else if (currentScreen == SPACE_MENU) {
      spaceMenuSelection += clockwise ? 1 : -1;

      if (spaceMenuSelection >= spaceMenuCount) {
        spaceMenuSelection = 0;
      }

      if (spaceMenuSelection < 0) {
        spaceMenuSelection = spaceMenuCount - 1;
      }

      drawSpaceMenu();
    }

    else if (currentScreen == PAUSED) {
      pauseSelection += clockwise ? 1 : -1;

      if (pauseSelection > 2) pauseSelection = 0;
      if (pauseSelection < 0) pauseSelection = 2;

      drawPauseMenu();
    }

    else if (currentScreen == ENCODER) {
      drawEncoder();
    }

    else if (currentScreen == WIFI_MANAGER) {
      int itemCount = availableNetworkCount + 2;

      selectedWifiItem += clockwise ? 1 : -1;

      if (selectedWifiItem >= itemCount) {
        selectedWifiItem = 0;
      }

      if (selectedWifiItem < 0) {
        selectedWifiItem = itemCount - 1;
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

  bool reading = digitalRead(ENC_SW);

  if (reading != lastButton) {
    lastButtonChange = millis();
    lastButton = reading;
  }

  if (millis() - lastButtonChange < DEBOUNCE_MS) {
    return;
  }

  if (reading == LOW && !buttonDown) {
    buttonDown = true;
    buttonDownAt = millis();
    longPressHandled = false;
    lastActivityTime = millis();

    if (currentScreen == SCREENSAVER) {
      wakeFromScreensaver();
      longPressHandled = true;
    }

    if (currentScreen == MAC_MEDIA) {
      mediaButtonDownAt = millis();
    }
  }

  // Long press handling
  if (buttonDown &&
      reading == LOW &&
      !longPressHandled &&
      millis() - buttonDownAt >= LONG_PRESS_MS) {

    longPressHandled = true;

    if (currentScreen == PLAYING) {
      currentScreen = PAUSED;
      pauseSelection = 0;
      drawPauseMenu();
    }

    else if (currentScreen == PAUSED ||
             currentScreen == GAME_OVER) {
      currentScreen = MENU;
      selected = 0;
      drawMenu();
    }

    else if (currentScreen == MAC_MEDIA) {
      currentScreen = MENU;
      selected = 0;
      drawMenu();
    }
  }

  // Button release
  if (reading == HIGH && buttonDown) {
    buttonDown = false;

    if (longPressHandled) {
      mediaButtonDownAt = 0;
      return;
    }

    if (currentScreen == MENU) {
      enterSelected();
    }

    else if (currentScreen == SPACE_MENU) {
      enterSpaceMenuItem();
    }

    else if (currentScreen == CALIBRATE_SENSOR) {
      performCalibration();
    }

    else if (currentScreen == SENSOR_INFO) {
      currentScreen = SPACE_MENU;
      drawSpaceMenu();
    }

    else if (currentScreen == SYSTEM ||
             currentScreen == ENCODER ||
             currentScreen == WEB_INFO) {
      currentScreen = MENU;
      drawMenu();
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

    else if (currentScreen == PLAYING) {
      shoot();
    }

    else if (currentScreen == PAUSED) {
      if (pauseSelection == 0) {
        currentScreen = PLAYING;
        lastFrame = millis();
      }

      else if (pauseSelection == 1) {
        startAfterCalibration = true;
        currentScreen = CALIBRATE_SENSOR;
        drawCalibration();
      }

      else {
        currentScreen = MENU;
        selected = 0;
        drawMenu();
      }
    }

    else if (currentScreen == GAME_OVER) {
      if (!calibrated) {
        startAfterCalibration = true;
        currentScreen = CALIBRATE_SENSOR;
        drawCalibration();
      } else {
        startGame();
      }
    }

    else if (currentScreen == MAC_MEDIA) {
      mediaButtonDownAt = 0;

      if (keyboard.isConnected()) {
        keyboard.tap(MEDIA_PLAY_PAUSE);
        mediaMessage = "Play/Pause sent";
      } else {
        mediaMessage = "Connect to Mac";
      }

      drawMacMedia();
    }
  }
}

// ================= SETUP =================

void setup() {
  pinMode(ENC_CLK, INPUT_PULLUP);
  pinMode(ENC_DT, INPUT_PULLUP);
  pinMode(ENC_SW, INPUT_PULLUP);

  lastCLK = digitalRead(ENC_CLK);
  lastButton = digitalRead(ENC_SW);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  SPI.begin(
    TFT_SCLK, -1, TFT_MOSI, TFT_CS
  );

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(35, 45);
  tft.println("NEXUS");

  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(35, 62);
  tft.println("Initializing...");

  // Initialize MPU6050
  chipID = 0xFF;
  readRegs(0x75, &chipID, 1);

  writeReg(0x6B, 0x00);
  writeReg(0x1C, 0x08);
  writeReg(0x1B, 0x08);
  writeReg(0x1A, 0x03);

  // Start Bluetooth HID
  keyboard.begin();

  // Wi-Fi setup
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(false);
  WiFi.persistent(false);
  WiFi.disconnect();

  // Web dashboard
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/data", HTTP_GET, handleApiData);
  server.begin();

  randomSeed(micros());
  initializeSaverStars();

  delay(300);

  currentScreen = MENU;
  drawMenu();
  lastActivityTime = millis();
}

// ================= MAIN LOOP =================

void loop() {
  handleEncoder();

  // Keep web dashboard responsive during gameplay.
  server.handleClient();

  // Idle screensaver (disabled on active game/calibration/Wi-Fi connection screens).
  updateScreensaver();

  // Live sensor information inside Space Shooter.
  if (currentScreen == SENSOR_INFO &&
      millis() - lastMotionUpdate >= 250) {
    lastMotionUpdate = millis();
    drawSensorInfo();
  }

  // Wi-Fi connection status.
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

  // Update Bluetooth connection status.
  bool bluetoothConnected = keyboard.isConnected();

  if (currentScreen == MAC_MEDIA &&
      bluetoothConnected != lastBluetoothState) {
    drawMacMedia();
  }

  lastBluetoothState = bluetoothConnected;

  // Space Shooter update.
  if (currentScreen == PLAYING &&
      millis() - lastFrame >= 33) {

    lastFrame = millis();

    updateGame();

    if (currentScreen == PLAYING) {
      drawGame();
    }
    else if (currentScreen == GAME_OVER) {
      drawGameOver();
    }
  }
}