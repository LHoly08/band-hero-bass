# band-hero-bass

ESP-IDF bass-controller firmware. The project requires CMake 3.31 or newer
and a toolchain with C++26 library support, including `std::saturating_cast`.

## Build

From the repository root in an activated ESP-IDF shell:

```sh
idf.py -D MAC=001122334455 build
```

Replace `001122334455` with the hub's MAC address: exactly 12 hexadecimal
digits, without colons. CMake remembers this value for later `idf.py build`
commands. The default `000000000000` is a placeholder.

## Source layout

- `include/Utils.hpp`: shared states, MAC and fret-value types, and button debounce.
- `main/Utils.cpp`: button interrupt and debounce implementation.
- `include/Strings.hpp`: ADC access and string input handling.
- `include/Playing.hpp`: hub and serial playing modes.
- `main/main.cpp`: startup and mode selection.

Startup selects hub mode. The configuration branch is a placeholder, and the
fret-value table in `main/main.cpp` is currently zero-initialized; fill it with
measured values for the instrument.

## C++ namespace convention

Qualify symbols defined in another file, even when they belong to the current
namespace: `bh::` for project symbols, `std::` for the standard library, and
`::` for APIs and types in the global namespace. Keep same-file names
unqualified when lookup permits; avoid `using namespace` directives.

Retain required class and enum scopes, such as `Play<S>::loop()` and
`State::Config`. Macros have no namespace and stay unqualified, including
`ESP_ERROR_CHECK`, `pdMS_TO_TICKS`, and `stdout`.
