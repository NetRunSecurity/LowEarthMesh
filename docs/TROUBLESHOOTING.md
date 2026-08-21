# Troubleshooting

Build/flash/boot issues for the L.E.M SAO (ESP32-C3 Super Mini + E22-900M22S), either build path (see [README](../README.md)).

| Symptom | Cause | Fix |
|---|---|---|
| `'Wire1' was not declared` (AutoOLEDWire.h) | C3 has a single I2C bus, the OLED driver expects a second one | `-D MESHTASTIC_EXCLUDE_SCREEN=1`, or base on `diy/esp32c3_super_mini` |
| `Unknown environment 'lem-...'` | typo in `[env:...]` header or folder not detected | check the header and the folder location |
| Compiles OK but radio is dead | `-I` still points at the old folder | fix `-I variants/esp32c3/lem` |
| `'pio' is not recognized` | plain PowerShell, not the PlatformIO PATH | use the PlatformIO Core CLI terminal, or the full path to `pio.exe` |
| Flash stuck on `Connecting....____` | auto-reset unreliable over native USB | BOOT + RST + release BOOT, retry |
| No serial log (board is alive) | console on UART0 | `-D ARDUINO_USB_MODE=1` + `-D ARDUINO_USB_CDC_ON_BOOT=1` |
| No BLE | board not booting (strapping) or WiFi init | fix the GPIO9 strapping issue, `WIFI_DISABLED=1` |
| Board dead, download mode only | GPIO9 strapping pin held low by an external signal | cut the trace and reroute to a non-strapping pin (V1 only) |
| `ClearCommError` / reconnect loop | pyserial + C3 USB-JTAG bug on Windows | Microsoft's Serial Monitor extension |
| Monitor drops on RST | RST also resets native USB | don't press RST while monitoring |
| COM port changes after reflash | firmware creates its own CDC device | recheck the port number |
| Panic / slow boot (filesystem) | LittleFS partition missing | `-t upload` (writes partitions) and/or `-t uploadfs` |

## Notes

- `WIFI_DISABLED=1`: necessity not confirmed. Applied alongside the GPIO9 hardware fix, the two haven't been isolated. Verify the flag is recognized by the build.
- Strapping-pin rule: never route an external signal with an uncertain reset-time state onto GPIO2, GPIO8, or GPIO9.
- Cold-boot reliability (v2 board, R1 pull-up on GPIO2): validated by repeated unplug/replug cycling on real hardware, 0 failures. See project history if a v2 board ever shows boot flakiness — that pull-up is the first thing to check.
