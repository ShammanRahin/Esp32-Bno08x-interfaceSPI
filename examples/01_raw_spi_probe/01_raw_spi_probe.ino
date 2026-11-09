// Raw SPI probe for the BNO08x, no library. ESP32 DevKit V1, 115200 baud.
// Resets the chip, waits for it to boot, then clocks out the first 4 bytes
// (the SHTP header) and prints them, so we can see whether the chip answers on SPI.
//
// Wiring: SCL->18  SDA->19 (MISO)  ADO->23 (MOSI)  CS->22  INT->21  RST->13
//         PS1->3V3  PS0->3V3  VIN->3V3  GND->GND

#include <Arduino.h>
#include <SPI.h>

#define PIN_SCK   18
#define PIN_MISO  19
#define PIN_MOSI  23
#define PIN_CS    22
#define PIN_INT   21
#define PIN_RST   13

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println("\n=== BNO08x raw SPI probe ===");

  pinMode(PIN_CS, OUTPUT);
  digitalWrite(PIN_CS, HIGH);
  pinMode(PIN_RST, OUTPUT);
  pinMode(PIN_INT, INPUT);
  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, -1);

  for (int round = 1; round <= 3; round++) {
    digitalWrite(PIN_RST, LOW);
    delay(20);
    digitalWrite(PIN_CS, HIGH);
    digitalWrite(PIN_RST, HIGH);
    delay(400);                                   // boot time
    Serial.printf("\nRound %d: INT level after 400 ms = %d (0 = chip has data for us)\n",
                  round, digitalRead(PIN_INT));

    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE3));
    digitalWrite(PIN_CS, LOW);
    delayMicroseconds(10);
    uint8_t h[4];
    for (int i = 0; i < 4; i++) h[i] = SPI.transfer(0x00);
    digitalWrite(PIN_CS, HIGH);
    SPI.endTransaction();

    Serial.printf("Header bytes: %02X %02X %02X %02X\n", h[0], h[1], h[2], h[3]);
    uint16_t len = (uint16_t)(h[0] | (h[1] << 8)) & 0x7FFF;
    Serial.printf("Length %u, channel %u, seq %u\n", len, h[2], h[3]);
    if (h[0] == 0xFF && h[1] == 0xFF && h[2] == 0xFF && h[3] == 0xFF)
      Serial.println("-> all FF: chip is not driving MISO (PS pins not high, SDA/MISO wire open, or CS wrong)");
    else if (h[0] == 0 && h[1] == 0 && h[2] == 0 && h[3] == 0)
      Serial.println("-> all 00: MISO stuck low, or no chip answering (check SDA/MISO and GND)");
    else if (h[2] <= 7 && len >= 4 && len < 2000)
      Serial.println("-> looks like a valid SHTP header: SPI works, the problem is elsewhere");
    else
      Serial.println("-> garbage: check SCL/SCK and ADO/MOSI wiring and SPI mode");
  }
  Serial.println("\nDone.");
}

void loop() {}
