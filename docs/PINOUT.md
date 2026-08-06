# L.E.M - Pinout reference

Reference pinouts for the two main modules, plus where they were sourced. Cross-check against [BOM.md](BOM.md) and the firmware's `variant/lem/variant.h`.

## E22-900M22S (SX1262)

22-pin module, stamp-hole castellated edge.

| Pin | Name | Direction | Remark |
|-----|------|-----------|--------|
| 1-5 | GND | - | Ground, connected to the power reference ground |
| 6 | RXEN | Input | RF switch receive control, connected to external MCU IO, active high |
| 7 | TXEN | Input | RF switch transmit control, connected to external MCU IO or DIO2, active high |
| 8 | DIO2 | Input/Output | Configurable general-purpose IO (see SX126x manual) |
| 9 | VCC | - | Power supply, 1.8V-3.7V. External ceramic filter capacitor recommended |
| 10-12 | GND | - | Ground, connected to the power reference ground |
| 13 | DIO1 | Input/Output | Configurable general-purpose IO (see SX126x manual) |
| 14 | BUSY | Output | Status indication (see SX126x manual) |
| 15 | NRST | Input | Chip reset trigger, active low |
| 16 | MISO | Output | SPI data output |
| 17 | MOSI | Input | SPI data input |
| 18 | SCK | Input | SPI clock |
| 19 | NSS | Input | Chip select, starts an SPI transaction |
| 20 | GND | - | Ground, connected to the power reference ground |
| 21 | ANT | - | Antenna interface, stamp hole, 50 ohm characteristic impedance |
| 22 | GND | - | Ground, connected to the power reference ground |

Source: https://www.aliexpress.com/item/1005010297464200.html

## ESP32-C3 Super Mini

| Left side | GPIO | | GPIO | Right side |
|-----------|------|---|------|------------|
| 5V | - | | GPIO5 | A5 / MISO |
| GND | - | | GPIO6 | MOSI |
| 3V3 | - | | GPIO7 | SS |
| A4 / SCK | GPIO4 | | GPIO8 | SDA |
| A3 | GPIO3 | | GPIO9 | SCL |
| A2 | GPIO2 | | GPIO10 | - |
| A1 | GPIO1 | | GPIO20 | RX |
| A0 | GPIO0 | | GPIO21 | TX |

GPIO2, GPIO8, and GPIO9 are strapping pins, see the strapping-pin rule in the [firmware setup guide](../README.md#notes).

Source: https://www.aliexpress.com/item/1005007205044247.html
