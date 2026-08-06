# L.E.M - Bill of Materials (Body v2)

Parts to assemble one **L.E.M SAO** board (ESP32-C3 Super Mini + E22-900M22S).
This covers assembly only. To have the PCB made you also need the Gerbers.

All SMD passives are **0805**.

## Body board

| Ref | Component | Value | Footprint | Qty | Source | Notes |
|-----|-----------|-------|-----------|-----|--------|-------|
| U1 | ESP32-C3 Super Mini | - | module | 1 | on hand | MCU + USB-C + LDO |
| IC1 | E22-900M22S | SX1262, 915 MHz | E22-900M22S | 1 | buy | LoRa module, U.FL onboard |
| SA01 | SAO connector | 2x3 2.54mm | PinHeader_2x03_SMD | 1 | custom | shrouded, SAO v1.69bis |
| C1 | Capacitor | 100nF | C_0805 | 1 | on hand | HF decoupling, closest to E22 pin 9 |
| C2 | Capacitor | 10µF | C_0805 | 1 | on hand | bulk, anti brown-out at TX. New in v2 |
| R1 | Resistor | 10kΩ | R_0805 | 1 | on hand | I2C SCL pull-up (GPIO2, strapping). New in v2 |
| R2 | Resistor | 10kΩ | R_0805 | 1 | on hand | I2C SDA pull-up (GPIO1). New in v2 |
| J1..J4 | Wing connectors | 1x2 2.00mm | PinHeader_1x02_P2.00mm | 4 | on hand | +3V3 / GND to wings |

## Required, not on the board

| Item | Spec | Notes |
|------|------|-------|
| Antenna | 915 MHz, U.FL / IPEX | Plugs into the E22. Fit it before any TX. Transmitting with no antenna can damage the PA. |
| USB-C cable | data capable | For flash, debug, bench power. v2 edge cut fits standard cables. |

## New in v2 (vs v1)

- C2 10µF bulk added on E22 VCC.
- R1, R2 10k I2C pull-ups added (SCL fixes the GPIO2 strapping risk at boot).
- BUSY reroute GPIO9 to GPIO0 formalized as a PCB trace (was a bodge wire).

---

## Wings (add-on, WIP)

Decorative solar-array wings, one PCB design flipped for the mirror side. LEDs always on, wired straight to +3V3.

> Status: schematic done, DRC not clean yet (no-net rectangles on F.Cu). Not ready to order.

Per wing:

| Ref | Component | Value | Footprint | Qty |
|-----|-----------|-------|-----------|-----|
| D1..D4 | LED | - | LED_0805 | 4 |
| R1..R4 | Resistor | 1kΩ | R_0805 | 4 |
| J1 | Connector + | 1x2 2.00mm | PinHeader_1x02_P2.00mm | 1 |
| J2 | Connector - | 1x2 2.00mm | PinHeader_1x02_P2.00mm | 1 |

---

*L.E.M - Low Earth Mesh // 低軌道メッシュ // NetRun Security 2026*
