# band-hero-bass

ESP-IDF bass-controller firmware. The project requires CMake 3.31 or newer
and a toolchain with C++26 library support, including `std::saturating_cast`,
and support for `-freflection`. Firmware C++ sources use C++26 with reflection
enabled.

## Build

From the repository root in an activated ESP-IDF shell:

```sh
idf.py -D MAC=001122334455 build
```

Replace `001122334455` with the hub's MAC address: exactly 12 hexadecimal
digits, without colons. CMake remembers this value for later `idf.py build`
commands. The default `000000000000` is a placeholder.

## Peer MAC over serial

At boot the bass prints its current receiver MAC. As on the stick, send a
complete command line to change it:

```text
M aabbccddeeff
```

Use exactly one space and 12 hexadecimal digits without colons, then Enter
(CR, LF, or CRLF). Letters are case-insensitive. In Arduino Serial Monitor,
enable a line ending. Invalid or oversized lines are discarded without changing
the address.

Commands work in configuration mode and while playing through the hub. In hub
mode, a successful command switches subsequent ESP-NOW sends immediately.
The address is saved as `receiver_mac` in the `bass` NVS namespace and restored
after reboot. The build-time `MAC` initializes NVS on first boot; subsequent
builds do not override a saved address. Erasing flash resets the saved setting.
If saving or registering a new peer fails, the current receiver remains active.

Use the configured primary serial console. The current `sdkconfig` uses UART0
at 115200 baud; to receive commands over native USB Serial/JTAG, select it as the
primary console in `idf.py menuconfig`. Serial playing mode keeps its binary
note output and does not process configuration commands.

## Source layout

- `include/Utils.hpp`: shared states, MAC and fret-value types, and button debounce.
- `main/Utils.cpp`: button interrupt and debounce implementation.
- `include/Strings.hpp`: ADC access and string input handling.
- `include/Playing.hpp`: hub and serial playing modes.
- `include/SerialCommand.hpp`: line-based peer MAC command parser.
- `include/PeerConfig.hpp`, `main/PeerConfig.cpp`: saved peer and serial updates.
- `main/main.cpp`: startup and mode selection.

Startup selects configuration mode, where peer MAC commands are accepted.
Mode selection is still unfinished; an `M` command saves the address and leaves
the mode unchanged. The fret-value table in `main/main.cpp` is currently
zero-initialized; fill it with measured values for the instrument.

Run the serial command parser checks on the host with:

```sh
g++ -std=c++26 -freflection -Wall -Wextra -Werror -Iinclude tests/SerialCommand.cpp -o /tmp/bass-serial-test
/tmp/bass-serial-test
```

## C++ namespace convention

Qualify symbols defined in another file, even when they belong to the current
namespace: `bh::` for project symbols, `std::` for the standard library, and
`::` for APIs and types in the global namespace. Keep same-file names
unqualified when lookup permits; avoid `using namespace` directives.

Retain required class and enum scopes, such as `Play<S>::loop()` and
`State::Config`. Macros have no namespace and stay unqualified, including
`ESP_ERROR_CHECK`, `pdMS_TO_TICKS`, and `stdout`.
