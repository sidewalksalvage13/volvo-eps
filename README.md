# Volvo EPS

Arduino firmware for controlling a Volvo Electric Power Steering (EPS) module over CAN bus. An Arduino Uno communicates with the EPS via an MCP2515 CAN controller, sending periodic speed commands and keep-alive messages. Speed is adjusted using the joystick on the CAN bus shield and persisted across power cycles via EEPROM.

## Hardware

| Component | Detail |
|---|---|
| Microcontroller | Arduino Uno (ATmega328P, 16 MHz) |
| CAN controller | MCP2515 via SPI — CS: pin 10, INT: pin 2 |
| CAN bus speed | 500 kbps |
| Joystick | 5-way on CAN shield (active LOW, internal pull-ups) |

### Joystick pinout

| Direction | Pin | Action |
|---|---|---|
| Up | A1 | Increase speed by 1000 |
| Left | A2 | Set speed to 0 |
| Down | A3 | Decrease speed by 1000 |
| Enter | A4 | Save current speed to EEPROM |
| Right | A5 | Set speed to max (6000) |

## Build & Flash

Requires [PlatformIO](https://platformio.org/).

```bash
platformio run                        # build
platformio run --target upload        # build and flash
platformio device monitor             # open serial monitor (115200 baud)
```

## How It Works

On startup the firmware loads the last saved speed from EEPROM (defaults to 0 if uninitialized). It then waits on the CAN bus for the EPS module's init message (`0x1B200002`). Once detected, it begins sending two messages on a fixed schedule:

- **Speed command** (`0x2104136`, extended, 8 bytes): sent every 15 ms. The current speed value (0–6000) is encoded big-endian in bytes 6–7.
- **Keep-alive** (`0x1AE0092C`, extended, 8 bytes): sent every 30 speed messages (~450 ms). A rolling counter (0–3) is encoded in the upper 2 bits of byte 0.

Speed can be adjusted at any time with the joystick. Changes take effect on the next CAN frame. Pressing Enter saves the current speed to EEPROM so it is restored on the next power-on.

## Dependencies

- [`coryjfowler/mcp_can@^1.5.0`](https://github.com/coryjfowler/MCP_CAN_lib) — MCP2515 CAN controller driver
- `<EEPROM.h>` — AVR built-in, no additional dependency
