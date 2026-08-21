# L.E.M - Low Earth Mesh 🛰️

Custom Meshtastic firmware and build/flash guide for the **L.E.M SAO** (Shitty Add-On), a LoRa Meshtastic node.

- **Target hardware**: ESP32-C3 Super Mini + E22-900M22S (SX1262), discrete/non-standard wiring.
- **Validated state**: firmware `2.8.0.8a0c759`, environment `lem-esp32c3-sx1262`, ROUTER node operational.

> **Why a custom build instead of the web flasher**: the LoRa pins are baked into `variant.h` at compile time and aren't configurable from the app. No pre-built target matches this discrete C3 + E22 wiring.

![Version 1 L.E.M](./assets/img01.jpg)

## Repository structure

```
LowEarthMesh/
├── README.md            this guide
├── LICENSE              GPL-3.0 (this project derives from Meshtastic firmware)
├── variant/lem/
│   ├── variant.h         reference pin/radio config, diff it against your own board
│   └── platformio.ini    reference build environment, diff it against your own board
└── docs/                 BOM, pinout, UART register map, troubleshooting — see docs/README.md
```

The `variant/lem/` files are a **reference**. Wiring can vary between individual builds (especially the GPIO9 strapping-pin bodge below), diff them against your own board before flashing.

## Which build do you want?

This repo supports two different builds off the same hardware. Pick one before you clone anything.

| | [Badge SAO](#path-a--badge-sao) | [Standalone SAO](#path-b--standalone-sao) |
|---|---|---|
| What it is | Full L.E.M: SAO firmware + `LemSaoModule`, exposes live mesh state to a host over a UART register map | Plain Meshtastic LoRa node on this hardware, nothing else |
| Base | [AlrikRr/Meshtastic-Firmware](https://github.com/AlrikRr/Meshtastic-Firmware) fork (`develop`) — variant + module already included | vanilla [meshtastic/firmware](https://github.com/meshtastic/firmware) + this repo's `variant/lem/` copied in |
| Pairs with | [badge-2026](https://github.com/AlrikRr/badge-2026) (NorthSec 2026 conference badge) over the SAO connector | nothing — a phone via BLE is all it needs |
| Use it if | you're building the badge-integrated demo | you just want a Meshtastic node on this board |

Both builds share the same [prerequisites](#0-prerequisites-windows) and [build/flash mechanics](#build--flash-both-paths) — only the source tree and starting point differ.

## 0. Prerequisites (Windows)

- **Git** (added to PATH during install).
- **VS Code** + **PlatformIO IDE** extension (bundles its own Python, no separate install needed).
- Optional but recommended: Microsoft's **Serial Monitor** extension (see [Monitoring](#serial-monitoring), PlatformIO's built-in monitor is flaky on the C3's USB-JTAG).

## Path A — Badge SAO

Clone the maintainer's fork — variant and `LemSaoModule` (the UART responder, register map documented in [`docs/REGISTER_MAP.md`](docs/REGISTER_MAP.md)) are already in the tree:

```sh
git clone --recurse-submodules -b develop https://github.com/AlrikRr/Meshtastic-Firmware.git
cd Meshtastic-Firmware
git submodule update --init
```

Open the folder in VS Code (the one containing the root `platformio.ini`), then jump to [Build & flash](#build--flash-both-paths) — environment `lem-esp32c3-sx1262`, nothing else to configure.

Once flashed, it talks over UART to a badge running [badge-2026](https://github.com/AlrikRr/badge-2026) (fork of [nsec/badge-2026](https://github.com/nsec/badge-2026), branch `feature-lem-sao`) — see [Badge integration](#badge-integration--northsec-2026) below for what that gets you, and that repo for its own build/flash steps.

## Path B — Standalone SAO

A plain Meshtastic LoRa node on this hardware — no `LemSaoModule`, no badge, no UART responder. Just this radio wiring on stock firmware.

1. Clone vanilla upstream:

   ```sh
   git clone --recurse-submodules https://github.com/meshtastic/firmware.git
   cd firmware
   git submodule update --init
   ```

2. Copy this repo's `variant/lem/` folder into the tree at `variants/esp32c3/lem/`.

   Starting from scratch instead of using the reference files here? Base it on one of:
   - **Recommended clean base**: `variants/esp32c3/diy/esp32c3_super_mini` (built for this board, already handles USB CDC, I2C, and the lack of a screen).
   - **Base used for the validated build**: `variants/esp32c3/heltec_esp32c3`, patched with the flags below.

3. `variants/esp32c3/lem/platformio.ini` (see [reference](variant/lem/platformio.ini)):

   ```ini
   [env:lem-esp32c3-sx1262]
   extends = esp32c3_base
   board = esp32-c3-devkitm-1
   build_flags =
     ${esp32c3_base.build_flags}
     -D PRIVATE_HW
     -D MESHTASTIC_EXCLUDE_SCREEN=1     ; no OLED on this board. Avoids the 'Wire1 not declared' bug (C3 has a single I2C bus)
     -D ARDUINO_USB_MODE=1              ; console on the native USB Serial/JTAG
     -D ARDUINO_USB_CDC_ON_BOOT=1       ; route Serial to USB CDC at boot. Otherwise logs go to UART0 and the USB terminal stays silent
     -D WIFI_DISABLED=1                 ; intended to free up BLE (not confirmed necessary, see docs/TROUBLESHOOTING.md)
     -I variants/esp32c3/lem
   monitor_speed = 115200
   upload_protocol = esptool
   upload_speed = 921600
   ```

   Key points if adapting: environment header `[env:lem-esp32c3-sx1262]`, identity `PRIVATE_HW` (no official model), and `-I` pointing at the new folder.

4. `variants/esp32c3/lem/variant.h` (see [reference](variant/lem/variant.h)):

   ```c
   #define HW_VENDOR meshtastic_HardwareModel_PRIVATE_HW
   #define USE_SX1262

   // SPI bus
   #define LORA_SCK   4
   #define LORA_MISO  5
   #define LORA_MOSI  6
   #define LORA_CS    7

   // SX1262 control
   #define SX126X_CS     7
   #define SX126X_RESET  3
   #define SX126X_DIO1   10
   #define SX126X_BUSY   0    // BODGE: was GPIO9. GPIO9 is a strapping pin, moved to GPIO0
   #define SX126X_RXEN   21   // RF switch RX (external E22 PA/LNA)
   #define SX126X_TXEN   20   // RF switch TX

   // E22: TCXO on DIO3, required or radio init fails
   #define SX126X_DIO3_TCXO_VOLTAGE 1.8
   #define TCXO_OPTIONAL              // fall back to XTAL if TCXO init fails

   // Super Mini builtin LED (active LOW)
   #define LED_PIN 8
   ```

   > Do **not** define `SX126X_DIO2_AS_RF_SWITCH`: TXEN/RXEN handle the switch here.
   > If the LED is inverted after flashing, add `#define LED_STATE_ON 0`.

Then continue to [Build & flash](#build--flash-both-paths).

## Build & flash (both paths)

Use the **PlatformIO Core CLI** terminal (alien icon > Open terminal, or the full path to `pio`). A plain PowerShell won't have `pio` on PATH.

```sh
pio run -e lem-esp32c3-sx1262
```

First build: 20-40 min (toolchain + framework download). Later builds are cached.
Ignore `warning:` lines. Only `*** Error` or `FAILED` matter.

Connect the board over **USB-C** (never SAO and USB-C at the same time). Then:

```sh
pio run -e lem-esp32c3-sx1262 -t upload --upload-port COM7
```

This writes the bootloader, partition table, and app together. The LittleFS partition formats itself on first boot. If needed, force the filesystem partition:

```sh
pio run -e lem-esp32c3-sx1262 -t uploadfs --upload-port COM7
```

**Entering download mode** (auto-reset is unreliable over native USB with `CDC_ON_BOOT`): if flashing hangs on `Connecting....____`, hold **BOOT**, tap **RST**, release **BOOT**, then retry.

## Serial monitoring

PlatformIO's built-in monitor can loop on `ClearCommError failed` against the C3's USB-JTAG on Windows. Two options:

- **Recommended**: VS Code **Serial Monitor** extension (Microsoft). Port COM7, baud 115200, Start Monitoring. It reconnects after USB re-enumeration.
- PlatformIO: `pio device monitor -e lem-esp32c3-sx1262 -p COM7` (less robust here).

Don't press the physical RST button while monitoring over native USB: RST also resets the USB controller and drops the connection.

**What to look for at boot**: `SX1262 init result 0` or equivalent (`result 0` = radio OK, SPI wiring is good) — and on Path A, `LemSao: UART ready (who_am_i=0x4C...)`.

## Network configuration (runtime)

Via the Meshtastic app over **BLE** (no screen on this board):

- **Region**: US (915MHz) — required regardless of path.
- **Role**: `ROUTER` is the validated config for badge pairing (Path A). Standalone (Path B) can use whatever role fits your deployment.

**Bluetooth pairing PIN** — differs by path, since only Path A's `LemSaoModule` overrides Meshtastic's default:

- **Path A**: `LemSaoModule` forces `RANDOM_PIN` every boot (in memory only). The SAO has no screen to show it — a paired badge reads it back over the `BLE_PIN` register and displays it on its e-ink. No config needed.
- **Path B**: stock Meshtastic default is `hasScreen ? RANDOM_PIN : FIXED_PIN` — this board has no screen, so it falls back to the static default PIN (`123456`) until changed:
  - Runtime: `meshtastic --set bluetooth.fixed_pin 654321` or via the app.
  - Compile-time (for a batch of boards): `userPrefs.jsonc` at the firmware root, key `"USERPREFS_FIXED_BLUETOOTH": "654321"`. Only applies to a board with no stored config (fresh flash or factory reset).

## Badge integration — NorthSec 2026

Path A only. The SAO talks over UART to [badge-2026](https://github.com/AlrikRr/badge-2026) (fork of [nsec/badge-2026](https://github.com/nsec/badge-2026), branch `feature-lem-sao`), NorthSec's ESP32-S3 conference badge — see [`docs/REGISTER_MAP.md`](docs/REGISTER_MAP.md) for the wire protocol. The badge reads live mesh state from the SAO and adds a full e-ink UI on top of it:

- **Dashboard** — live node count, packets relayed, RSSI/SNR, last message, and the docked SAO's own name.
- **Message** — the last received text, DM or broadcast.
- **SIGINT Constellation + Target Lock** — every node heard gets classified by role (Interceptor / Orbital Relay / Deep Space Probe / Unidentified Anomaly) and hop distance; lock onto one and a NeoPixel radar pulses faster/redder as its signal strengthens.
- **LemDex** — a persistent, NVS-backed "Tactical Space RPG" collection of every node ever discovered, browsable as filterable cards.
- **DM notifications** — a distinct NeoPixel flash when a private message (not a broadcast) arrives.
- **Sleep mode** — long-press to blank the LEDs and e-ink for a low-key standby state.
- **QR code screen** — a tap-friendly contact card, also reachable via a long-press shortcut from any screen.
- **Dock/disconnect feedback** — ambient NeoPixel + e-ink screens when the SAO is plugged in or pulled.
- **Orbital Docking** — bump two badges together over NFC to exchange their SAO's public key/contact info directly, no waiting on mesh NodeInfo propagation.
- **Bluetooth pairing PIN relay** — see [Network configuration](#network-configuration-runtime) above.

All of the above is badge-side (`badge-2026`) logic built on top of the register map the SAO exposes — the SAO firmware in this repo stays deliberately lean.

## Troubleshooting

Moved to [`docs/TROUBLESHOOTING.md`](docs/TROUBLESHOOTING.md) — symptom table plus a couple of standing notes (WiFi flag, strapping pins, cold-boot).

## License

This project derives from [Meshtastic firmware](https://github.com/meshtastic/firmware) and is distributed under **GPL-3.0** (see [LICENSE](LICENSE)).

## Official resources

| Doc | URL |
|---|---|
| Build firmware from source | https://meshtastic.org/docs/development/firmware/build/ |
| Flashing firmware | https://meshtastic.org/docs/getting-started/flashing-firmware/ |
| Bluetooth settings | https://meshtastic.org/docs/configuration/radio/bluetooth/ |
| Meshtastic firmware repo | https://github.com/meshtastic/firmware |
| `userPrefs.jsonc` | https://github.com/meshtastic/firmware/blob/master/userPrefs.jsonc |
