# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Arduino firmware for interfacing with a **Volvo Electric Power Steering (EPS)** system over CAN bus. The ATmega328P (Arduino Uno) communicates with the EPS module via an MCP2515 CAN controller at 500 kbps.

## Build & Development Commands

This project uses **PlatformIO** as the build system.

```bash
# Build the firmware
platformio run

# Upload to Arduino
platformio run --target upload

# Open serial monitor (115200 baud)
platformio device monitor

# Clean build artifacts
platformio run --target clean

# Run unit tests (test/ dir currently empty)
platformio test -e uno

# Run a single test filter
platformio test -e uno --filter=<test_name>

# Static analysis
platformio check --severity=medium
```

## Architecture

All application logic lives in `src/main.cpp`. The firmware operates in three phases:

1. **Initialization** (`setup()`): Configures Serial at 115200 baud, joystick pins (INPUT_PULLUP), CAN pins, initializes MCP2515 at 500 kbps, and loads `eps_speed` from EEPROM (validates range; defaults to 0 if uninitialized).

2. **Connection detection**: Polls the CAN interrupt pin waiting for message ID `0x1B200002` from the EPS module. Sets `eps_connected = true` on receipt.

3. **Control loop** (`loop()`): Polls joystick inputs on every iteration, then once connected sends two CAN messages on a fixed schedule:
   - **Speed message** (ID `0x2104136`, extended): Every 15 ms. Speed (0–6000) encoded big-endian in bytes 6–7.
   - **Keep-alive message** (ID `0x1AE0092C`, extended): Every 30 speed messages (~450 ms). Rolling counter (0–3) in the upper 2 bits of byte 0.

### Joystick / debounce pattern

Button handling uses edge-detection debounce via `handleButton()`: fires the action only on the HIGH→LOW transition, after `DEBOUNCE_MS` (50 ms) has elapsed since the last state change. Per-button state (`joy_state_*`) and timestamp (`joy_last_ms_*`) globals track this. Do not use level-based debounce (checking `== LOW` in a loop) — it fires repeatedly while held.

`eps_speed` is persisted to EEPROM address 0 (2 bytes, `uint16_t`) on Enter press via `EEPROM.put`, which only writes if the value changed.

## Hardware

- **Board**: Arduino Uno (ATmega328P, 16 MHz)
- **CAN controller**: MCP2515 module connected via SPI
  - CS: pin 10
  - INT: pin 2
- **CAN bus speed**: 500 kbps
- **Library**: `coryjfowler/mcp_can@^1.5.0`
- **Joystick** (active LOW, INPUT_PULLUP): Up=A1, Left=A2, Down=A3, Enter=A4, Right=A5
- **EEPROM**: Built-in AVR (`<EEPROM.h>`, no lib_deps entry needed); address 0–1 reserved for `eps_speed`

## CAN Message Reference

| Message | ID | Direction | Notes |
|---|---|---|---|
| EPS init signal | `0x1B200002` | RX | Triggers `eps_connected = true` |
| Speed command | `0x2104136` | TX | Bytes 6–7 = speed (0–6000), 15 ms interval |
| Keep-alive | `0x1AE0092C` | TX | Byte 0 upper 2 bits = rolling counter, ~450 ms interval |

All IDs use extended CAN format (29-bit).
