# ESP32 + BNO08x over SPI (SparkFun BNO08x library)

A working SPI wiring and test sketches for a BNO08x IMU (BNO080 / BNO085 / BNO086) on a classic ESP32,
plus notes on the problems I hit and how I got past them.

This is a write-up of what worked on my setup, not an official guide. Where I don't know the cause of something, I say so.

## What this was tested on

| Item | Value |
|---|---|
| Board | DOIT ESP32 DevKit V1 (ESP32-WROOM) |
| Arduino core | arduino-esp32 `[fill in version]` |
| Library | SparkFun BNO08x Arduino Library `1.0.6` |
| IMU module | GY-BNO08x-style breakout, pins: VIN, GND, SCL, SDA, ADO, CS, INT, RST, PS0, PS1 (check your own silkscreen) |

**Not tested:** the Adafruit BNO08x library, I²C or UART modes, other ESP32 variants (S2, S3, C3), other core versions.

## Wiring

| Module pin | SPI role | ESP32 |
|---|---|---|
| VIN | power | 3V3 |
| GND | ground | GND |
| SCL | SCK | GPIO18 |
| SDA | MISO | GPIO19 |
| ADO | MOSI | GPIO23 |
| CS | chip select | GPIO22 |
| INT | interrupt | **GPIO21** |
| RST | reset | GPIO13 |
| PS0 | mode select | 3V3 |
| PS1 | mode select | 3V3 |

Notes:

- PS0 and PS1 must both be high at reset to select SPI. On many breakouts these pads are pulled or jumpered to GND by default, which gives I²C.
- On this kind of board the pins have I²C names. In SPI mode **SCL is SCK, SDA is MISO, and ADO is MOSI**. ADO is not left floating or tied to a level.
- **Do not put INT on GPIO34-39.** See below.

## The INT pin problem

With INT on GPIO34, `beginSPI()` never returned: nothing printed after the call, no error, no reboot.
A raw SPI read with no library returned a valid start-up packet on the same wiring, and `digitalRead` on GPIO34 saw INT go low, so the chip and the pin were fine.

Moving INT to GPIO21 and changing nothing else made `beginSPI()` return `true` and events streamed at about 200 per second.

GPIO34-39 on the ESP32 are input-only and have no internal pull-ups. **I do not know why the library hangs on them.**
A SparkFun community post (ESP32 Thing Plus C with a BNO086 over SPI) reports the same behaviour with INT on GPIO36.

## Troubleshooting

| Symptom | What it turned out to be |
|---|---|
| `beginSPI` returns `false` in about 0.2 s | SCL/SDA wires on the wrong pins, or PS0/PS1 not high |
| Raw probe reads `FF FF FF FF` | Chip not driving MISO: not in SPI mode, or the SDA/MISO wire is not reaching the ESP32 |
| `beginSPI` never returns | INT on an input-only pin (34-39). Moving it to GPIO21 fixed it here |
| Status lines print once, then stop | An unbounded `while (imu.getSensorEvent())` loop. Capping it at 10 events per pass fixed it. Cause not isolated |
| INT reads 0 before a reset | The chip had unread data from the previous run. Harmless |

## Examples

- `examples/01_raw_spi_probe`: no library. Resets the chip and prints the first 4 bytes it sends. A valid header (for example `14 01 00 00`, channel 0) means SPI wiring and mode are correct.
- `examples/02_sparkfun_bno08x_spi`: the SparkFun library. Prints yaw, pitch, roll, gyro and accelerometer 10 times a second.

Expected output once it works (board lying roughly flat):

```
OK: BNO08x found. Keep it still first, then tilt it.
yaw     1.0 pitch   -4.1 roll  -14.3 | gyro    0.1    0.0    0.1 dps | acc  0.69 -2.41  9.39 m/s2 | 0 ev/s
yaw     1.0 pitch   -4.0 roll  -14.3 | gyro    0.0    0.0    0.0 dps | acc  0.69 -2.41  9.30 m/s2 | 195 ev/s
```

The first line shows `0 ev/s` because the counter updates after one second.

## Using it in your own firmware

- Read one event per `loop()` pass, or cap the count, so the rest of the loop keeps running.
- Keep INT off GPIO34-39 when you design the PCB.
- Wait about 500 ms after `beginSPI` before enabling reports, as the examples do.

## Questions

Open an issue. If something here is wrong or doesn't match your setup, say what board, core version and library version you use.
