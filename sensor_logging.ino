#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_TSL2561_U.h>
#include <Adafruit_CCS811.h>
#include <DHT.h>
#include <LowPower.h>

// -------------------- DHT22 --------------------
#define DHTPIN 2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// -------------------- BMP280 -------------------
Adafruit_BMP280 bmp;

// -------------------- TSL2561 ------------------
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);
bool tslFound = false;

// -------------------- CCS811 -------------------
Adafruit_CCS811 ccs;

// -------------------- Setup helpers --------------------
void configureTSL2561() {
  tsl.enableAutoRange(true);
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);
}

void setup() {
  Serial.begin(9600);
  delay(1000);

  Serial.println("Starting sensor test...");
  Serial.println();

  Wire.begin();

  // DHT22
  dht.begin();

  // BMP280
  if (!bmp.begin(0x76)) {
    Serial.println("BMP280 not found!");
    while (1);
  }

  // TSL2561 — optional, program continues without it
  if (!tsl.begin()) {
    Serial.println("TSL2561 not found, skipping light readings.");
  } else {
    configureTSL2561();
    tslFound = true;
  }

  // CCS811
  if (!ccs.begin()) {
    Serial.println("CCS811 not found!");
    while (1);
  }

  Serial.print("Waiting for CCS811 to be ready");
  while (!ccs.available()) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();

  Serial.println("All sensors initialised!");
  Serial.println();
}

void loop() {
  // ---------------- Wake CCS811 up ----------------
  ccs.setDriveMode(CCS811_DRIVE_MODE_1SEC);
  delay(2000);

  // ---------------- DHT22 ----------------
  float dhtHumidity = dht.readHumidity();
  float dhtTempC = dht.readTemperature();

  // ---------------- BMP280 ----------------
  float bmpTempC = bmp.readTemperature();
  float pressure_hPa = bmp.readPressure() / 100.0;

  // ---------------- TSL2561 ----------------
  float lux = 0;
  bool luxValid = false;

  if (tslFound) {
    sensors_event_t event;
    tsl.getEvent(&event);
    if (event.light) {
      lux = event.light;
      luxValid = true;
    }
  }

  // ---------------- CCS811 ----------------
  int eCO2 = -1;
  int TVOC = -1;

  if (!isnan(dhtHumidity) && !isnan(dhtTempC)) {
    ccs.setEnvironmentalData(dhtHumidity, dhtTempC);
  }

  if (ccs.available()) {
    if (!ccs.readData()) {
      eCO2 = ccs.geteCO2();
      TVOC = ccs.getTVOC();
    } else {
      Serial.println("CCS811 read error!");
    }
  }

  // ---------------- Print Results ----------------
  Serial.println("========== Sensor Readings ==========");

  // DHT22
  if (isnan(dhtHumidity) || isnan(dhtTempC)) {
    Serial.println("DHT22: Failed to read");
  } else {
    Serial.print("DHT22 Temp: ");
    Serial.print(dhtTempC);
    Serial.println(" C");

    Serial.print("Humidity: ");
    Serial.print(dhtHumidity);
    Serial.println(" %");
  }

  // BMP280
  Serial.print("BMP280 Temp: ");
  Serial.print(bmpTempC);
  Serial.println(" C");

  Serial.print("Pressure: ");
  Serial.print(pressure_hPa);
  Serial.println(" hPa");

  // TSL2561
  if (!tslFound) {
    Serial.println("Light: TSL2561 not connected");
  } else if (luxValid) {
    Serial.print("Light: ");
    Serial.print(lux);
    Serial.println(" lux");
  } else {
    Serial.println("Light: Sensor overload or no reading");
  }

  // CCS811
  if (eCO2 >= 0 && TVOC >= 0) {
    Serial.print("eCO2: ");
    Serial.print(eCO2);
    Serial.println(" ppm");

    Serial.print("TVOC: ");
    Serial.print(TVOC);
    Serial.println(" ppb");
  } else {
    Serial.println("CCS811: No valid reading yet");
  }

  Serial.println("====================================");
  Serial.println();

  // ---------------- Put CCS811 to idle ----------------
  ccs.setDriveMode(CCS811_DRIVE_MODE_IDLE);

  // ---------------- 30-minute low-power sleep ----------------
  Serial.println("Sleeping for 30 minutes...");
  Serial.flush();

  // 30 min = 1800s / 8s per cycle = 225 cycles
  for (int i = 0; i < 225; i++) {
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  }
}
