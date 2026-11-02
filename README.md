# T-Display S3 DNS Adblock (safe test build)

Flashable PlatformIO firmware for the **LILYGO T-Display S3 (ESP32-S3, 16 MB
flash)**. It answers blocklisted A/AAAA DNS requests with a zero address and
relays non-blocklisted requests to one configured upstream resolver.

This is a small, intentionally conservative port inspired by the documented
architecture of [M-Abozaid/esp32-c3-adblock](https://github.com/M-Abozaid/esp32-c3-adblock): domains are encoded as sorted 40-bit FNV-1a hashes in LittleFS, so lookup needs little RAM. It is an independent implementation; see [third-party notices](THIRD_PARTY_NOTICES.md).

> **Status: compile-ready configuration; hardware unverified.** The original
> upstream tested an ESP32-C3, not this board. This project has not yet been
> flashed to a physical T-Display S3, so the LCD pin offsets, buttons, radio,
> and DNS behavior require on-device validation. PlatformIO was unavailable in
> the authoring environment, so a local compile was not run either.

## What it does / deliberately does not do

| Included | Intentionally absent |
| --- | --- |
| Local blocklist compiler, UDP DNS relay, suffix blocking, LCD statistics | DHCP, captive portal, web UI, remote blocklist download, OTA firmware update |
| One configured upstream IPv4 resolver | Automatic router/DHCP changes or network-wide DNS redirection |
| One-client IPv4 allow-list | TLS, authentication, multi-client service |

The omissions are security choices for a first firmware build. A DNS appliance
is highly privileged: it sees requests and can disrupt every client using it.

## LCD and controls

The screen shows connection state, ESP IP, blocklist status, DNS safety state,
and counters. Press **BOOT** briefly to switch between the overview and stats
page. The auxiliary button is reserved and currently has no effect. Holding
BOOT while reset may still enter the ROM bootloader, as expected.

The code assumes common T-Display S3 wiring (ST7789: SCLK 18, MOSI 17, CS 39,
DC 38, RST 37, backlight 48; buttons 0 and 14). Treat this as unverified until
tested on the exact board revision.

## Prerequisites (Linux + VS Code)

- **VS Code:** already installed on your machine. Open this project's folder,
  containing `platformio.ini`, with **File > Open Folder**.
- **PlatformIO IDE extension:** open Extensions (`Ctrl+Shift+X`), search for
  **PlatformIO IDE** by PlatformIO, and install it. Let its initial setup finish.
  The extension includes PlatformIO Core; a separate global `pip` installation
  and Arduino IDE are unnecessary. See the
  [official VS Code setup guide](https://docs.platformio.org/en/latest/integration/ide/vscode.html).
- **Python 3 and virtual-environment support:** required for the blocklist
  builder and PlatformIO's setup. On Debian/Ubuntu, install with
  `sudo apt install python3 python3-venv git`. Other distributions use their
  equivalent packages. Git is useful for version control and repository-based
  dependencies.
- **Internet access for the first build:** PlatformIO downloads the ESP32
  toolchain, Arduino framework, and display library automatically.
- **USB permissions:** follow PlatformIO's
  [Linux udev setup](https://docs.platformio.org/en/latest/core/installation/udev-rules.html),
  then unplug/reconnect the board. If access is still denied, inspect the
  serial device's owning group and follow the documented group-membership
  procedure; group changes require logging out and back in. Run VS Code and
  PlatformIO as your normal user.
- **Hardware:** a LILYGO T-Display S3 with **16 MB flash**, a USB data cable
  (not a charging-only cable), and a stable USB power source. Other LILYGO S3
  models may need different display wiring and configuration.
- **Network:** a trusted **2.4 GHz Wi-Fi** network, its SSID/password, and one
  test client with a reserved IPv4 address. Put that address in
  `ALLOWED_CLIENT_IP` before enabling DNS.
- **Local configuration:** copy `src/config.example.h` to `src/config.h`,
  enter Wi-Fi credentials, and leave DNS disabled for the first boot.
- **Blocklist:** a reviewed, locally saved domain/hosts list. Generate
  `data/blocklist.bin` before uploading the filesystem.

To check the setup, open **PlatformIO > Quick Access > Miscellaneous >
PlatformIO Core CLI** in VS Code and run:

```sh
python3 --version
pio --version
pio device list
```

The board must be connected to appear in the last command. Use this PlatformIO
terminal for the build/flash commands below if `pio` is unavailable in your
ordinary Linux terminal. The project already selects its board configuration;
you do not need to create a new PlatformIO project.

## Configure, build, and flash

1. Make the local configuration and set the Wi-Fi network. Keep it untracked.

   ```sh
   cp src/config.example.h src/config.h
   ```

2. Build a blocklist from a reviewed local text file. Supported inputs are one
   domain per line, `0.0.0.0 domain` hosts rows, and basic `||domain^` rules.

   ```sh
   python3 tools/build_blocklist.py data/blocklist.bin path/to/reviewed-list.txt
   ```

3. Flash firmware and the filesystem. Connect the board while it is in normal
   boot mode; use the port PlatformIO reports when more than one exists.

   ```sh
   pio run -e t-display-s3 -t upload
   pio run -e t-display-s3 -t uploadfs
   pio device monitor -b 115200
   ```

4. Verify the LCD has joined Wi-Fi and note its IP. **Do not enable the DNS
   server yet.** At this point it only displays status.

## Safe DNS test procedure

1. In `src/config.h`, set `ALLOWED_CLIENT_IP` to the fixed IPv4 address of one
   spare test device. Reserve that address in the router first so it does not
   change.
2. Set `DNS_SERVICE_ENABLED` to `1`; keep the upstream as a trusted resolver.
3. Reflash firmware. Do not change router DHCP DNS, and do not point more than
   that test client at the device.
4. On only the test client, set DNS to the ESP's IP. Check a benign allowed
   domain resolves, then check a domain you knowingly placed in your local
   list. Watch the LCD counters and serial output.
5. Restore the test client's normal DNS immediately if forwarding, latency, or
   application behavior is unexpected. Reflash with `DNS_SERVICE_ENABLED 0` to
   stop listening completely.

The service refuses packets from other IPv4 clients. That is a guardrail, not
strong authentication: LAN traffic remains unencrypted and DNS can be spoofed
on an untrusted network.

## Blocklist format and limitations

`blocklist.bin` consists of sorted, big-endian five-byte hashes. It blocks a
domain and its parents: blocking `example.com` also blocks
`cdn.example.com`. A 40-bit hash has a small collision risk, which can cause a
false block. Rules involving regexes, wildcards, cosmetic filters, modifiers,
and allow-list exceptions are excluded by the compiler. The firmware blocks
only A and AAAA queries; other query types are forwarded.

## Validation checklist

Before relying on this beyond a bench test, validate all of the following:

- `pio run` succeeds with the installed PlatformIO/espressif32 version.
- LCD rotation, offsets, backlight, and both stated buttons work on the exact
  board revision.
- A normal DNS name forwards successfully; a known list entry returns a zero
  A or AAAA answer; non-allowed client traffic increments `rejected`.
- Power-cycle recovery, Wi-Fi reconnect, list upload, and the failure mode when
  the list is absent are acceptable.
- Resolver latency and router/client behavior are acceptable on a test VLAN or
  isolated test client.

Only after those checks should you consider a separate, reviewed deployment
design for additional clients. Do not expose UDP/53 to the internet.
