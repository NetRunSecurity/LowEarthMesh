# L.E.M - UART register map (v0.8)

Contract exposed by the SAO (Meshtastic `LemSaoModule`, UART slave) to any host reading it
over the connector's two signal pins. Deliberately **host-agnostic**: a host discovers a
L.E.M SAO via `WHO_AM_I` and doesn't need to know anything else about the host firmware
that's polling it. The reference host implementation is `badge-2026` (`lib/core/hardware/lem_sao.*`).

## Transport (UART, v1.1)

The SAO connector's two signal pins carry UART instead of I2C (SAO side: `GPIO1=TX`,
`GPIO2=RX`; direction convention on the host side is documented in the host's own pinout).
100kHz-I2C-equivalent target throughput, 115200 baud in practice — a full read-only block
(~80 bytes + frame overhead) takes ~7-8ms, well under a 3Hz host poll rate.

Registers are 8-bit, addressed explicitly per frame (no auto-increment pointer):

```
Request  (host -> SAO): [0xAA START] [CMD] [REG] [LEN] [PAYLOAD if CMD=WRITE] [CRC8]
Response (SAO -> host):
  READ:  [0x55 START] [STATUS] [LEN] [DATA] [CRC8]
  WRITE: [0x55 START] [STATUS] [0x00 LEN] [CRC8]
```

- `CMD`: `0x01` = READ, `0x02` = WRITE.
- `REG`: starting register (same address space as the table below).
- `LEN`: number of bytes to read/write (max useful value: `0x50`, the full read-only block;
  `0x41` = 65 for a full `TX_LEN`+`TX_BUF` write).
- `CRC8`: CRC-8/CCITT (poly `0x07`) over every preceding byte in the frame.
- `STATUS` (response): `0x00` = OK, non-zero = error.
- Distinct start bytes (`0xAA` request / `0x55` response) let either side resync by scanning
  the stream if a previous frame was misread.
- Expected robustness on the host side: a strict per-read timeout, RX buffer purge before
  each new request, and a retry budget with a consecutive-failure counter before declaring
  the SAO unreachable (see "Disconnect behavior" below).

## Register table

```
Read-only:
0x00        WHO_AM_I      = 0x4C
0x01        FW_VERSION    major<<4 | minor
0x02        STATUS        bit0 mesh joined / bit1 RX pending / bit2 TX busy / bit3 radio fault /
                          bit4 RX_IS_DM (v0.4) / bit5 BLE_PAIRING (v0.6)
0x03        NODE_COUNT
0x04        PKT_RELAYED_L u16 LE, wraps
0x05        PKT_RELAYED_H
0x06        LAST_RSSI     i8 dBm -- updated on every promiscuous packet
0x07        LAST_SNR      i8 dB x4 -- updated on every promiscuous packet
0x08        RX_LEN        text payload length -- updated only if RX_PORTNUM = TEXT_MESSAGE_APP
0x09-0x0C   RX_FROM       u32 LE, full NodeNum -- updated on every promiscuous packet (v0.2)
0x0D        RX_ROLE       u8 -- raw meshtastic_Config_DeviceConfig_Role, 0xFF = not yet resolved (v0.2)
0x0E        RX_PORTNUM    u8 -- raw meshtastic_PortNum, saturates to 0xFF if > 255 (v0.2)
0x0F        HOPS_AWAY     u8 -- hop_start - hop_limit, 0xFF = indeterminate (v0.3)
0x10-0x4F   RX_BUF        64 bytes UTF-8, text only, filled if RX_PORTNUM = TEXT_MESSAGE_APP

Write:
0x50        TX_LEN        1-64
0x51-0x90   TX_BUF
0x91        TX_COMMIT     write 0x01 = broadcast (ignored if TX busy)
0x92-0x9F   OWN_LONG_NAME 14 bytes, read-only, UTF-8, not NUL-terminated on the wire --
                          truncated copy of this SAO's own long name (v0.7)

0xA0-0xA3   DOCK_NODENUM  u32 LE -- target NodeNum for NFC out-of-band key injection (v0.5)
0xA4-0xC3   DOCK_PUBKEY   32 bytes -- target X25519 public key (v0.5)
0xC4        DOCK_COMMIT   write 0x01 = stage async NodeDB commit + favorite (v0.5, see below)

0xC5-0xC8   OWN_NODENUM   u32 LE, read-only -- this SAO's own NodeNum (v0.5)
0xC9-0xE8   OWN_PUBKEY    32 bytes, read-only -- this SAO's own X25519 public key (v0.5)
0xE9-0xED   OWN_SHORT_NAME 5 bytes, read-only, UTF-8 -- this SAO's own short name (v0.7)

0xF0        CTRL          bit0 clear counters
0xF1-0xF4   BLE_PIN       u32 LE, read-only -- Bluetooth pairing passkey, valid only while
                          STATUS.BLE_PAIRING is set (v0.6)
0xF5-0xF6   OWN_BLE_SUFFIX 2 bytes, read-only -- last 2 MAC bytes, same suffix used in this
                          SAO's advertised Bluetooth device name (v0.8)
```

`0x00-0x0F` is the identity/status header block, shared convention for any SAO in this
project family (a host discovers capability via `WHO_AM_I`). No unallocated bytes remain in
that block as of v0.4.

`OWN_NODENUM`/`OWN_PUBKEY` (0xC5-0xE8) are **read-only** despite sitting address-wise inside
the 0xA0+ block alongside the write-only `DOCK_*` registers -- listed in address order rather
than moved up next to the other read-only block to keep the docking-related registers
grouped together. A host needs to read these before a docking exchange to know what identity
to hand over on its own behalf (previously an unexposed gap, see Version history v0.5).

`0xA4-0xC3` (`DOCK_PUBKEY`) holds an X25519 public key -- Meshtastic's PKC direct-message
encryption uses X25519 (Diffie-Hellman key exchange), **not** Ed25519 (signing). Do not
confuse the two when implementing either side.

## Synchronization rule (since v0.2)

`LAST_RSSI` / `LAST_SNR` / `RX_FROM` / `RX_ROLE` / `RX_PORTNUM` / `HOPS_AWAY` all update
together on every promiscuous packet, so a single host read always sees a coherent tuple from
one packet. `RX_LEN` / `RX_BUF` stay text-only (only updated when `RX_PORTNUM =
TEXT_MESSAGE_APP`). `STATUS.RX_IS_DM` (v0.4) joins the text-only group: recomputed (set or
cleared) on every promiscuous packet, but only meaningful when `RX_PORTNUM =
TEXT_MESSAGE_APP`.

**Known limitation, accepted as best-effort**: `LAST_RSSI` only updates when the packet
carries a radio metric (`mp.has_rx_rssi`). A locally-generated/looped packet (e.g. a
`ROUTING_APP` ACK the SAO itself originates) still advances `RX_FROM`/`RX_ROLE`/`RX_PORTNUM`/
`HOPS_AWAY`, but leaves `LAST_RSSI` stale from the previous radio packet. The same packet
type also reports `HOPS_AWAY` as indeterminate. No fix planned -- treat as sampling noise, the
same category as the 3Hz poll rate itself.

**No per-device addressing.** `WHO_AM_I` is a fixed byte on every L.E.M SAO -- the protocol
assumes exactly one SAO on the bus. Two SAOs sharing the same physical signal pins is not
supported (see the host's own hardening notes for what that requires).

## NFC Docking commit (v0.5)

Writing `0x01` to `DOCK_COMMIT` (`0xC4`) does **not** perform the NodeDB injection
synchronously within that write transaction. It stages `DOCK_NODENUM`/`DOCK_PUBKEY` and sets
an internal flag, drained on the SAO's own next background tick rather than inline in the
UART responder path.

**Why**: the actual injection (`NodeDB::commitRemoteKey()` + `set_favorite()` +
`saveToDisk()`) ends in a synchronous, blocking flash write that acquires the SPI bus lock --
the same lock the LoRa radio driver uses for its own SPI transactions. Performing that write
inline inside the time-critical UART request/response handler risks starving the radio's SPI
access and blowing the UART side's own per-frame timeout budget. This is not a deadlock risk
(single lock, no circular wait), but a latency/starvation one -- worth stating precisely so a
future contributor doesn't chase the wrong failure mode.

**Host implication**: a `DOCK_COMMIT` write acknowledges once staged, not once applied. A host
that needs to confirm the contact was actually committed should re-read relevant state after a
short delay rather than assume success is immediate.

## Write-only status bits

`STATUS.RX_PENDING` (bit1) has no clear/ack mechanism -- it only ever gets set, never cleared,
by the SAO. A host that needs edge-detection (e.g. "new message since last poll") has to track
`(RX_FROM, RX_LEN)` itself between polls rather than relying on this bit. `CTRL` (`0xF0`) only
exposes a "clear counters" bit, not a way to acknowledge pending-message state.

## Disconnect behavior (host-side hardening expectations)

A host driving this register map over UART should treat the SAO as hot-pluggable and design
for these failure modes:

- **Per-frame timeout**: a bounded wait for the response start byte (tens of ms, well above
  the expected round-trip at 115200 baud) so a disconnect mid-transaction degrades to a clean
  timeout, not a hang.
- **Immediate retry budget**: a few retries within one read/write call absorb an isolated
  dropped/corrupted frame (CRC mismatch, stale response from a previous timed-out request)
  without escalating to "SAO gone."
- **Consecutive-failure threshold across polls**: only declare the SAO unreachable after
  several consecutive failed polls, not on the first miss -- avoids flapping on a transient
  glitch while still detecting a real unplug promptly.
- **Bounded bus lock**: if the transport is shared with another peripheral/protocol on the
  same physical pins (as on the reference host, see its own docs), guard bus acquisition with
  a bounded wait rather than blocking forever, so one stuck caller can't freeze every other
  consumer of the bus.

The reference host implementation (`badge-2026`) follows exactly this shape and additionally
surfaces the transition visually (LED + status screen) once the failure threshold is crossed,
then backs off to a slower re-probe interval until `WHO_AM_I` answers again.

## Bluetooth PIN relay (v0.6)

The L.E.M SAO has no screen of its own, so Meshtastic's default-config logic
(`NodeDB.cpp`, `hasScreen ? RANDOM_PIN : FIXED_PIN`) would otherwise leave Bluetooth pairing on
the static default PIN forever. `LemSaoModule` now forces `config.bluetooth.mode = RANDOM_PIN`
in memory on every boot (not persisted), and relays the actual pairing passkey via `BLE_PIN` +
`STATUS.BLE_PAIRING` so a host can display it during pairing -- the same job a Meshtastic
device's own screen does via `screen->startAlert()` (see `NimbleBluetooth.cpp`
`onPassKeyNotify()`). `BLE_PIN` sits outside the `0x00-0x4F` block `WHO_AM_I`-block reads cover
in one transaction, so a host needs a dedicated read for it (same situation as `OWN_NODENUM`/
`OWN_PUBKEY`, v0.5).

## Own name (v0.7)

`OWN_LONG_NAME`/`OWN_SHORT_NAME` expose this SAO's own Meshtastic display name (`owner.long_name`/
`owner.short_name`) so a host can label which physical SAO is docked -- useful once more than one
SAO exists. Refreshed every tick like `OWN_NODENUM`/`OWN_PUBKEY` (v0.5), no `STATUS` validity bit
needed (static identity, not a transient state). `OWN_LONG_NAME` is a **truncated** 14-byte copy
of `owner.long_name` (buffer is 40 bytes, Meshtastic's own UI convention already caps it at 24
UTF-8 bytes) -- there was no single free contiguous register-map gap large enough for the full
field, and 14 characters is enough for the small e-ink line this drives. `OWN_SHORT_NAME` is a
raw 5-byte copy (already short by Meshtastic's own design, no truncation needed).

## BLE name suffix (v0.8)

`OWN_BLE_SUFFIX` exposes the same last-2-MAC-bytes suffix `getDeviceName()` (`main.cpp`) appends
to this SAO's advertised Bluetooth device name (`<short_name>_xxxx`, or `Meshtastic_xxxx` if
short_name is unset). This is **not** derivable from `OWN_NODENUM` or `OWN_LONG_NAME`/
`OWN_SHORT_NAME` -- confirmed against real hardware (2026-08-18) that the BLE name suffix comes
from the MAC address, not the Meshtastic NodeNum, which is a distinct value. Exists specifically
so a host can build an identifier that matches what a user actually sees when scanning for this
SAO over Bluetooth -- useful as a fallback when `OWN_SHORT_NAME` itself isn't displayable (e.g.
a non-ASCII short_name a host's font can't render).

## Version history

- **v0.1**: initial block, `WHO_AM_I`..`RX_BUF`, single-byte `RX_FROM`.
- **v0.2**: `RX_FROM` widened to 4 bytes; `RX_ROLE`/`RX_PORTNUM` added; RSSI/SNR/FROM/ROLE/
  PORTNUM synchronization rule introduced (fixes a desync where a stale `RX_FROM` could pair
  with an unrelated, more recent `LAST_RSSI`).
- **v0.3**: `HOPS_AWAY` added.
- **v0.4**: `STATUS` bit4 (`RX_IS_DM`) added for private-message detection.
- **v0.5**: `DOCK_NODENUM`/`DOCK_PUBKEY`/`DOCK_COMMIT` added for NFC out-of-band contact/key
  exchange ("Orbital Docking") -- replaces the earlier `CHANNEL_*` channel-exchange draft,
  which is dropped in favor of per-node PKC key injection (no channel switch needed, the SAO
  stays on Primary). Also adds `OWN_NODENUM`/`OWN_PUBKEY` (read-only) -- caught during
  implementation prep: nothing let a host read its own SAO's identity to hand over during a
  docking exchange, a gap that predates this feature (previously tracked as a deferred idea,
  "a real per-SAO ID," in `plan-dev.md`) but that Orbital Docking makes load-bearing rather
  than optional.
- **v0.6**: `BLE_PIN` + `STATUS` bit5 (`BLE_PAIRING`) added to relay the Bluetooth pairing
  passkey to a host, now that `LemSaoModule` forces `RANDOM_PIN` pairing mode every boot instead
  of inheriting the screen-less default (`FIXED_PIN`).
- **v0.7**: `OWN_LONG_NAME`/`OWN_SHORT_NAME` added so a host can identify which physical SAO is
  docked by its configured Meshtastic display name.
- **v0.8**: `OWN_BLE_SUFFIX` added -- the BLE device name suffix isn't derivable from
  `OWN_NODENUM` or v0.7's name registers (it comes from the MAC), and a host needs it to build a
  fallback identifier that matches what's seen when scanning for this SAO over Bluetooth.
