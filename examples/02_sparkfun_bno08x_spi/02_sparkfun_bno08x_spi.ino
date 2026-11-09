// BNO08x over SPI with the SparkFun BNO08x library, classic ESP32 (DOIT ESP32 DevKit V1)
// Library: "SparkFun BNO08x Arduino Library" (Arduino Library Manager)
// Serial Monitor: 115200 baud
//
// Wiring (module pin -> ESP32):
//   VIN -> 3V3      GND -> GND
//   SCL -> GPIO18 (SCK)    SDA -> GPIO19 (MISO)    ADO -> GPIO23 (MOSI)
//   CS  -> GPIO22   INT -> GPIO21   RST -> GPIO13
//   PS1 -> 3V3      PS0 -> 3V3
//
// IMPORTANT: INT is on GPIO21, not on an input-only pin (GPIO34-39).
// With INT on GPIO34, beginSPI() never returned on this setup.

#include <Arduino.h>
#include <SPI.h>
#include <SparkFun_BNO08x_Arduino_Library.h>

#define PIN_SCK   18
#define PIN_MISO  19
#define PIN_MOSI  23
#define PIN_CS    22
#define PIN_INT   21
#define PIN_RST   13

BNO08x imu;

float yaw = 0, pitch = 0, roll = 0;
float gx = 0, gy = 0, gz = 0;
float ax = 0, ay = 0, az = 0;
uint32_t eventCount = 0, eventsPerSec = 0, lastRate = 0, lastPrint = 0;

void enableReports() {
  imu.enableGameRotationVector();
  imu.enableGyro(20);             // ms between reports
  imu.enableAccelerometer(20);
}

void quatToEuler() {
  float qi = imu.getQuatI(), qj = imu.getQuatJ(), qk = imu.getQuatK(), qr = imu.getQuatReal();
  yaw   = atan2f(2.0f * (qr * qk + qi * qj), 1.0f - 2.0f * (qj * qj + qk * qk)) * RAD_TO_DEG;
  float s = constrain(2.0f * (qr * qj - qk * qi), -1.0f, 1.0f);
  pitch = asinf(s) * RAD_TO_DEG;
  roll  = atan2f(2.0f * (qr * qi + qj * qk), 1.0f - 2.0f * (qi * qi + qj * qj)) * RAD_TO_DEG;
}

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println("\n=== BNO08x SPI test (SparkFun BNO08x library) ===");

  // CS is driven by the library as a plain GPIO, so pass -1 as the SPI SS pin
  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, -1);

  Serial.println("calling beginSPI");
  bool ok = imu.beginSPI(PIN_CS, PIN_INT, PIN_RST, 3000000, SPI);
  Serial.printf("beginSPI returned %s\n", ok ? "true" : "false");
  if (!ok) {
    Serial.println("FAIL: BNO08x not found over SPI. See the README troubleshooting table.");
    while (true) delay(1000);
  }

  delay(500);
  enableReports();
  delay(100);
  imu.getSensorEvent();
  Serial.println("OK: BNO08x found. Keep it still first, then tilt it.");
  lastRate = millis();
}

void loop() {
  if (imu.wasReset()) {
    Serial.println("Sensor was reset, re-enabling reports");
    enableReports();
  }

  // At most 10 events per pass so the print below always runs.
  // An unbounded `while (imu.getSensorEvent())` starved the rest of loop() in testing.
  for (int n = 0; n < 10 && imu.getSensorEvent(); n++) {
    eventCount++;
    switch (imu.getSensorEventID()) {
      case SENSOR_REPORTID_GAME_ROTATION_VECTOR:
        quatToEuler();
        break;
      case SENSOR_REPORTID_GYROSCOPE_CALIBRATED:
        gx = imu.getGyroX() * RAD_TO_DEG;
        gy = imu.getGyroY() * RAD_TO_DEG;
        gz = imu.getGyroZ() * RAD_TO_DEG;
        break;
      case SENSOR_REPORTID_ACCELEROMETER:
        ax = imu.getAccelX();
        ay = imu.getAccelY();
        az = imu.getAccelZ();
        break;
    }
  }

  uint32_t now = millis();
  if (now - lastRate >= 1000) {
    eventsPerSec = eventCount;
    eventCount = 0;
    lastRate = now;
  }
  if (now - lastPrint >= 100) {
    lastPrint = now;
    Serial.printf("yaw %7.1f pitch %6.1f roll %6.1f | gyro %6.1f %6.1f %6.1f dps | acc %5.2f %5.2f %5.2f m/s2 | %lu ev/s\n",
                  yaw, pitch, roll, gx, gy, gz, ax, ay, az, (unsigned long)eventsPerSec);
  }
}
