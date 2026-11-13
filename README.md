# band-hero-bass

ESP-IDF bass-controller firmware in development, with ESP-NOW hub and
binary serial playing modes. Requires CMake 3.31 or newer and a toolchain with
C++26 library support, including `std::saturating_cast`, and compiler support
for `-freflection` and `<meta>`.

## Build and flash

From the repository root in an activated ESP-IDF shell, select the target for
your board on a fresh checkout. The local development configuration uses
ESP32-S3; CMake does not force a target, and `sdkconfig` is not tracked.
For an ESP32-S3 board:

```sh
idf.py set-target esp32s3
idf.py -DMAC=001122334455 build
```

Replace `001122334455` with the hub's Wi-Fi station MAC address: exactly
12 hexadecimal digits, without colons. CMake remembers it for subsequent
builds. The default `000000000000` is a placeholder. Configure the flash size
and primary console for your board with `idf.py menuconfig`.

Flash and monitor using your board's actual serial port:

```sh
idf.py -p /dev/ttyUSB0 flash monitor
```

## Peer MAC over serial

At boot the firmware prints its current receiver MAC. Send a complete command
line to change it:

```text
M aabbccddeeff
```

Use exactly one space and 12 hexadecimal digits without colons, then Enter
(CR, LF, or CRLF). Command letters and hexadecimal digits are case-insensitive.
In Arduino Serial Monitor, enable a line ending. Invalid or oversized lines
are discarded without changing the address.

Commands are polled in configuration mode and while playing through the hub.
In hub mode, a successful command switches subsequent ESP-NOW sends immediately.
The address is saved as `receiver_mac` in the `bass` NVS namespace and restored
after reboot. The build-time `MAC` initializes NVS on first boot; subsequent
builds do not override a saved address. Erasing flash resets the saved setting.
If saving or registering a new peer fails, the current receiver remains active.
NVS recovery from a full partition or incompatible stored format erases the
partition and restores the build-time default.

Use the configured primary serial console. The local development `sdkconfig`
uses UART0 at 115200 baud; select USB Serial/JTAG as the primary console in
`idf.py menuconfig` if commands should arrive over native USB. Serial playing
mode writes binary notes to stdout and does not process configuration commands.
Sender and hub must use the same Wi-Fi channel; the firmware does not explicitly
select one.

## Current behavior and limitations

Startup enters configuration mode and accepts peer MAC commands. `main/main.cpp`
constructs the hub and serial players in their respective state branches, but
there is no configuration action that enters either playing state yet. An `M`
command saves the address and leaves the mode unchanged. While playing, the
GPIO20 rising-edge button returns to configuration mode with 200 ms debounce.
Bluetooth is not implemented.

`include/Strings.hpp` generates four string members for GPIO1–GPIO4 using
C++26 reflection. Fret lookup uses a shared 24-entry table and a tolerance of
100 ADC counts, taking the first match. The table in `main/main.cpp` is currently
zero-initialized and needs measured fret values. Every string currently reads
ADC1 channel 0, and ADC channel configuration is missing; independent string
sampling remains unfinished.

Playing modes send a four-byte note value when it changes, over ESP-NOW or
binary serial. Each string uses five bits starting at bit `2 + 5 * stringIndex`.
The current loop ORs readings into the previous note value without clearing old
fret bits, so note transitions and releases still need work. Wi-Fi startup also
needs the network-interface and default event-loop initialization that the
entry point currently lacks. The firmware is not yet a complete playable
controller.

## Source layout

- `include/Utils.hpp`: atomic mode state, MAC and fret-value types, and button debounce.
- `main/Utils.cpp`: button interrupt and debounce implementation.
- `include/Strings.hpp`: ADC access, fret lookup, and generated string members.
- `include/Playing.hpp`: hub and serial playing modes.
- `include/SerialCommand.hpp`: line-based peer MAC command parser.
- `include/PeerConfig.hpp`, `main/PeerConfig.cpp`: saved receiver and serial updates.
- `main/main.cpp`: startup and mode dispatch.

## Validation

This repository currently has no host test suite. Firmware builds require the
C++26 reflection toolchain described above; hardware validation is still needed
for input sampling, mode transitions, and hub delivery.

## C++ namespace convention

Qualify symbols defined in another file, even when they belong to the current
namespace: `bh::` for project symbols, `std::` for the standard library, and
`::` for APIs and types in the global namespace. Keep same-file names
unqualified when lookup permits; avoid `using namespace` directives.

Retain required class and enum scopes, such as `Play<S>::loop()` and
`State::Config`. Macros have no namespace and stay unqualified, including
`ESP_ERROR_CHECK`, `pdMS_TO_TICKS`, and `stdout`.
