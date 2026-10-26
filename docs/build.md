# Build & run

## Requirements

- [PlatformIO](https://platformio.org/) — the Teensy platform and toolchain are fetched
  automatically
- A Teensy 4.1 on a Bionic base board
- `teensy-cli` for flashing (installed by the PlatformIO Teensy platform)

Dependencies are declared in `platformio.ini` and resolved by PlatformIO:

| Library | Version | Role |
|---|---|---|
| [`tgtakaoka/libasm`](https://github.com/tgtakaoka/libasm) | 2.0.0 | Assembler and disassembler for every target |
| [`tgtakaoka/libcli`](https://github.com/tgtakaoka/libcli) | 1.5.1 | Serial command-line interface |

## Build and flash

```sh
pio run                 # build the teensy41 environment
pio run -t upload       # build and flash
```

The default environment is `teensy41`; `teensy41-profile` adds
`-D PROFILE_CYCLES`, `teensy41-stress` adds `-D STRESS_HALTGO`, `teensy41-clock` adds
`-D TUNE_CLOCK`, and `native` runs the host tests:

```ini
[env:teensy41]
platform = teensy
board = teensy41
framework = arduino
build_flags = -D TEENSY_OPT_FAST_LTO -D USB_DUAL_SERIAL
upload_protocol = teensy-cli
```

`src_dir` and `include_dir` both point at [`debugger/`](../debugger).

## Serial ports

`USB_DUAL_SERIAL` gives the Teensy **two** USB serial ports, and they do different jobs:

| Port | Role |
|---|---|
| First (`Serial`) | The debugger console — this is where you type commands |
| Second (`SerialUSB1`) | **Software HALT** — sending any character here breaks into a running program |

Both run at **115200 8N1**. The second port is the software equivalent of the HALT/RUN
switch on the base board: any byte arriving on it fires `serialEventUSB1()`, which calls
`Pins::isrHaltSwitch()` (`debugger/main.cpp`). That is what lets you stop a program started
with `G` without reaching for the board.

Connect with whatever you like — the recorded sessions use `minicom`:

```sh
minicom -D /dev/ttyACM0 -b 115200
```

## First contact

Plug a CPU adapter into the ZIF socket before powering up: the firmware reads the board's
identity EEPROM at boot and configures itself for that processor. Press `?` for the banner,
the registers and the list of commands:

```
* BionicMC6809 * 1.0
```

If the identity cannot be read, or you hold the user switch low during reset, the firmware
starts with a null target. `W` writes a new identity string to the board's EEPROM — the
input must be prefixed with `+`.

Then load a program with `U` and paste in one of the `.hex` or `.s19` files from
[`samples/`](../samples). See [samples.md](samples.md).

## Editor setup

`clangd` needs a compilation database:

```sh
make compiledb           # runs: pio run -e teensy41 -t compiledb
```

This regenerates `compile_commands.json`. Re-run it after switching branches or changing
dependencies. `.clangd` in the repository root forces `-xc++` on `.h` files so headers are
parsed as C++.

The database is also generated automatically before each build by
[`scripts/setup_compiledb.py`](../scripts/setup_compiledb.py), except in CI.

## Continuous integration

[`.github/workflows/platformio-ci.yml`](../.github/workflows/platformio-ci.yml) installs both
libraries, runs the host tests (`pio test -e native`) and `pio ci -c platformio.ini -e teensy41
debugger` on every push and pull request.
