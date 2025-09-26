// ================= RS485 + LCD + Blynk (ESP32) =================
// ----- User: AbuDahm RS485 testing + Blynk push + MIN/MAX input -----

// -------------------- Blynk Setup --------------------
#define BLYNK_TEMPLATE_ID "TMPL6hCCxyQg5"
#define BLYNK_TEMPLATE_NAME "RS485 LCD Blynk ESP32"
#define BLYNK_AUTH_TOKEN "iG46AT_CPEEDD8AFzOBdZDS72nhrlGsr"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Arduino.h>

// -------------------- RS485 / LCD --------------------
#include <ModbusMaster.h>
#include <HardwareSerial.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

// Serial debug and RS485 pins
#define SerialDebug        Serial
#define SerialRS485_RX_PIN 26
#define SerialRS485_TX_PIN 27
#define SerialRS485        Serial2

// Modbus slave IDs
#define WEATHER_ID 1
#define SOIL_ID    2

// LCD initialization (20x4)
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Flags to check connections
bool wifiConnected = false;
bool blynkConnected = false;

// WiFi credentials
const char* WIFI_SSID = "iQOO Neo9";
const char* WIFI_PASS = "88888888";

// Timings for sensor read and Blynk push
const unsigned long sensorInterval = 1000;
const unsigned long pushInterval   = 2000;

// Modbus objects
ModbusMaster weatherSensor;
ModbusMaster soilSensor;

// Sensor values
float soilHumi, soilTemp, ec, ph, nitrogen, phosphorus, potassium, salinity, tds;
float airHumi, airTemp, lux;

// Timers
unsigned long lastSensorRead = 0;
unsigned long lastBlynkPush  = 0;

// Blynk timer
BlynkTimer timer;

// Blynk virtual pins
#define VP_AH   V0
#define VP_AT   V1
#define VP_LUX  V2
#define VP_SH   V3
#define VP_ST   V4
#define VP_EC   V5
#define VP_pH   V6
#define VP_N    V7
#define VP_P    V8
#define VP_K    V9
#define VP_S    V10
#define VP_TDS  V11
#define VP_AIR_MIN   V40
#define VP_AIR_MAX   V41
#define VP_SOIL_MIN  V42
#define VP_SOIL_MAX  V43

// Default temperature setpoints and hysteresis
float airMin  = 22.0f, airMax  = 28.0f;
float soilMin = 18.0f, soilMax = 26.0f;
const float HYST = 0.5f;

// I2C LCD connection checker
bool lcdExists(uint8_t addr) {
  Wire.begin();
  Wire.beginTransmission(addr);
  return (Wire.endTransmission() == 0);
}

// -------------------- Setup --------------------
void setup() {
  SerialDebug.begin(9600);
  delay(500);
  SerialDebug.println("=== System Startup ===");

  // LCD check and init
  Wire.begin();
  Wire.setClock(100000);
  if (!lcdExists(0x27)) {
    SerialDebug.println("[ERROR] LCD not detected at 0x27.");
    while (true);
  }
  lcd.begin();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("LCD OK");

  // WiFi connection
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  SerialDebug.printf("Connecting to WiFi: %s ...\n", WIFI_SSID);
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 8000) {
    delay(500);
    SerialDebug.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    SerialDebug.println("\nWiFi Connected.");
    lcd.setCursor(0, 1); lcd.print("WiFi OK         ");
  } else {
    SerialDebug.println("\n[WARNING] WiFi NOT connected.");
    lcd.setCursor(0, 1); lcd.print("WiFi FAIL       ");
  }

  // Blynk connection
  if (wifiConnected) {
    Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASS);
    delay(1000);
    if (Blynk.connected()) {
      blynkConnected = true;
      SerialDebug.println("Blynk Connected.");
      lcd.setCursor(0, 2); lcd.print("Blynk OK        ");
      timer.setInterval(pushInterval, printToBlynk);
      Blynk.syncVirtual(VP_AIR_MIN, VP_AIR_MAX, VP_SOIL_MIN, VP_SOIL_MAX);
    } else {
      SerialDebug.println("[WARNING] Blynk NOT connected.");
      lcd.setCursor(0, 2); lcd.print("Blynk FAIL      ");
    }
  }

  // RS485 init
  SerialRS485.begin(4800, SERIAL_8N1, SerialRS485_RX_PIN, SerialRS485_TX_PIN);
  weatherSensor.begin(WEATHER_ID, SerialRS485);
  soilSensor.begin(SOIL_ID, SerialRS485);

  lcd.setCursor(0, 3);
  lcd.print("RS485 + Ready");
}

// -------------------- Main Loop --------------------
void loop() {
  unsigned long t = millis();
  if (t - lastSensorRead >= sensorInterval) {
    readWeatherSensor();
    delay(200);
    readSoilSensor();
    lastSensorRead = t;
    printToDebugger();
    controlAirSoilByRanges();
    printToLcd();
    printUserSetpoints();
  }

  if (wifiConnected && blynkConnected) {
    Blynk.run();
    timer.run();
  }
}

// Combine two 16-bit registers to form 32-bit value
static inline uint32_t combine_u16_to_u32(uint16_t hi, uint16_t lo) {
  return ((uint32_t)hi << 16) | (uint32_t)lo;
}

// -------------------- Read Weather Sensor --------------------
void readWeatherSensor() {
  uint8_t result = weatherSensor.readHoldingRegisters(0x01F4, 9);
  if (result == weatherSensor.ku8MBSuccess) {
    airHumi = weatherSensor.getResponseBuffer(0) / 10.0f;
    airTemp = weatherSensor.getResponseBuffer(1) / 10.0f;
    uint16_t lux_hi = weatherSensor.getResponseBuffer(6);
    uint16_t lux_lo = weatherSensor.getResponseBuffer(7);
    uint32_t lux_u32 = combine_u16_to_u32(lux_hi, lux_lo);
    lux = (float)lux_u32;

    // ========== Debug LUX Info ==========
    SerialDebug.println("---- LUX DEBUG ----");
    SerialDebug.printf("lux_hi (16-bit): %u\n", lux_hi); // lux_hi (1) = lux (32-bit) 65536
    SerialDebug.printf("lux_lo (16-bit): %u\n", lux_lo);
    SerialDebug.printf("lux_32bit      : %lu\n", lux_u32);
    SerialDebug.println("--------------------");
  }
}

// -------------------- Read Soil Sensor --------------------
void readSoilSensor() {
  uint8_t result = soilSensor.readHoldingRegisters(0x0000, 9);
  if (result == soilSensor.ku8MBSuccess) {
    soilHumi    = soilSensor.getResponseBuffer(0) / 10.0f;
    soilTemp    = soilSensor.getResponseBuffer(1) / 10.0f;
    ec          = soilSensor.getResponseBuffer(2);
    ph          = soilSensor.getResponseBuffer(3) / 10.0f;
    nitrogen    = soilSensor.getResponseBuffer(4);
    phosphorus  = soilSensor.getResponseBuffer(5);
    potassium   = soilSensor.getResponseBuffer(6);
    salinity    = soilSensor.getResponseBuffer(7);
    tds         = soilSensor.getResponseBuffer(8);
  }
}

// -------------------- Serial Debug Print --------------------
void printToDebugger() {
  SerialDebug.println("----Weather Sensor-----");
  SerialDebug.printf("Air Humidity   : %0.1f %%\n", airHumi);
  SerialDebug.printf("Air Temperature: %0.1f C\n", airTemp);
  SerialDebug.printf("Light          : %0.1f lux\n", lux);

  SerialDebug.println("------Soil Sensor------");
  SerialDebug.printf("Soil Humidity  : %0.1f %%\n", soilHumi);
  SerialDebug.printf("Soil Temp      : %0.1f C\n", soilTemp);
  SerialDebug.printf("EC             : %0.0f uS/cm\n", ec);
  SerialDebug.printf("pH             : %0.1f\n", ph);
  SerialDebug.printf("Nitrogen (N)   : %0.0f mg/kg\n", nitrogen);
  SerialDebug.printf("Phosphorus (P) : %0.0f mg/kg\n", phosphorus);
  SerialDebug.printf("Potassium (K)  : %0.0f mg/kg\n", potassium);
  SerialDebug.printf("Salinity       : %0.0f mg/L\n", salinity);
  SerialDebug.printf("TDS            : %0.0f mg/L\n", tds);
  SerialDebug.println("----------END -----------");
  // Send all values as CSV line to RS485 for listener
  SerialRS485.printf("%.1f,%.1f,%.0f,%.1f,%.1f,%.0f,%.1f,%.0f,%.0f,%.0f,%.0f,%.0f\n",
                   airHumi, airTemp, lux,
                   soilHumi, soilTemp, ec, ph,
                   nitrogen, phosphorus, potassium, salinity, tds);
}

// -------------------- LCD Print --------------------
void printToLcd() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.printf("AH:%.1f AT:%.1f L:%.0f", airHumi, airTemp, lux);

  lcd.setCursor(0, 1);
  lcd.printf("SH:%.1f ST:%.1f T:%.0f", soilHumi, soilTemp, tds);

  lcd.setCursor(0, 2);
  lcd.printf("pH:%.1f EC:%.0f S:%.0f", ph, ec, salinity);

  lcd.setCursor(0, 3);
  lcd.printf("N:%.0f P:%.0f K:%.0f", nitrogen, phosphorus, potassium);
}

// -------------------- Blynk Push --------------------
void printToBlynk() {
  Blynk.virtualWrite(VP_AH, airHumi);
  Blynk.virtualWrite(VP_AT, airTemp);
  Blynk.virtualWrite(VP_LUX, lux);
  Blynk.virtualWrite(VP_SH, soilHumi);
  Blynk.virtualWrite(VP_ST, soilTemp);
  Blynk.virtualWrite(VP_EC, ec);
  Blynk.virtualWrite(VP_pH, ph);
  Blynk.virtualWrite(VP_N, nitrogen);
  Blynk.virtualWrite(VP_P, phosphorus);
  Blynk.virtualWrite(VP_K, potassium);
  Blynk.virtualWrite(VP_S, salinity);
  Blynk.virtualWrite(VP_TDS, tds);
}

// -------------------- Blynk Write Handlers --------------------
BLYNK_WRITE(VP_AIR_MIN) { float v = param.asFloat(); airMin = (v >= airMax) ? airMax - 0.5f : v; }
BLYNK_WRITE(VP_AIR_MAX) { float v = param.asFloat(); airMax = (v <= airMin) ? airMin + 0.5f : v; }
BLYNK_WRITE(VP_SOIL_MIN){ float v = param.asFloat(); soilMin= (v >= soilMax)? soilMax - 0.5f : v; }
BLYNK_WRITE(VP_SOIL_MAX){ float v = param.asFloat(); soilMax= (v <= soilMin)? soilMin + 0.5f : v; }

// -------------------- User Setpoints Debug --------------------
void printUserSetpoints() {
  SerialDebug.printf("[SETPOINTS] AirMin=%.1f | AirMax=%.1f | SoilMin=%.1f | SoilMax=%.1f\n",
                      airMin, airMax, soilMin, soilMax);
}

// -------------------- Control Logic --------------------
void controlAirSoilByRanges() {
  if (airTemp < airMin - HYST) SerialDebug.println("[CTRL] AirTemp < Min → LOW");
  else if (airTemp > airMax + HYST) SerialDebug.println("[CTRL] AirTemp > Max → HIGH");
  else SerialDebug.println("[CTRL] AirTemp in range");

  if (soilTemp < soilMin - HYST) SerialDebug.println("[CTRL] SoilTemp < Min → LOW");
  else if (soilTemp > soilMax + HYST) SerialDebug.println("[CTRL] SoilTemp > Max → HIGH");
  else SerialDebug.println("[CTRL] SoilTemp in range");
}