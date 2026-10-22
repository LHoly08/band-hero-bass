# band-hero-bass

Build from an ESP-IDF shell:

```sh
idf.py -D MAC=001122334455 build
```

Replace `001122334455` with the hub's MAC address: exactly 12 hexadecimal
digits, without colons. CMake remembers this value for later `idf.py build`
commands. The default `000000000000` is a placeholder.

