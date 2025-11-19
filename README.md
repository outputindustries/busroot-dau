# Busroot DAU Firmware

Industrial-grade Data Acquisition Unit (DAU) firmware for Arduino Opta, featuring dual-core architecture with robust input handling, MQTT communication, and 1000-frame circular buffer for maximum reliability.

## Features

### Hardware Support
- **Arduino Opta** (STM32H747 dual-core)
- **M4 Core**: Real-time input reading and debouncing
- **M7 Core**: Network communication and data processing

### Input Capabilities
- 6 digital inputs with 50ms debouncing and edge counting
- 2 analog inputs (12-bit, 0-4095 range)
- User button monitoring
- Falling-edge detection for pulse counting

### Communication
- **MQTT** support via WiFi, Ethernet, or Blues Wireless
- **Modbus RTU** support for energy meters (19200 baud)
- **Serial console** for configuration and debugging
- Valid JSON message formatting

### Reliability Features
- **1000-frame circular buffer** on M4 core (83 minutes @ 5s intervals)
- **Watchdog protection** on both cores
- **Lock-free circular buffer** for safe dual-core communication
- **Race-condition-free** counter implementation
- **Configuration persistence** in flash memory
- **Automatic MQTT reconnection** on connection failure

## Quick Start

### 1. Build Firmware

```bash
# Install PlatformIO CLI
pip install platformio

# Clone repository
git clone <your-repo-url>
cd busroot-dau

# Build both cores
pio run

# Firmware binaries automatically copied to web/firmwares/
```

### 2. Flash to Device

```bash
# Upload M7 core
pio run -e opta_m7 -t upload

# Upload M4 core
pio run -e opta_m4 -t upload
```

### 3. Configure Device

1. Connect to serial console (19200 baud)
2. Press Enter within 10 seconds of boot
3. Follow configuration prompts:
   - Communication mode (WIFI/ETHERNET/BLUES/NONE)
   - Network credentials
   - MQTT broker settings
   - Modbus configuration (optional)

## Project Structure

```
busroot-dau/
├── src/
│   ├── m4.cpp              # M4 core: Input reading
│   ├── m7.cpp              # M7 core: Networking & MQTT
│   ├── data_frame.h/cpp    # Inter-core communication
│   ├── config.h/cpp        # Configuration management
│   └── status.h/cpp        # Status & error handling
├── web/
│   ├── index.html          # Web interface
│   ├── css/styles.css      # Styling
│   ├── js/app.js           # JavaScript
│   └── firmwares/          # Auto-generated binaries
├── platformio.ini          # Build configuration
├── copy_firmware.py        # Post-build script
└── README.md               # This file
```

## Architecture

### M4 Core (Input Processing)
- Runs input polling loop at maximum speed
- Debounces all inputs with 50ms delay
- Counts falling edges on digital inputs
- Reads analog values (12-bit resolution)
- Writes data frames to circular buffer in SDRAM every 5 seconds
- Manages buffer overflow by dropping oldest frames
- Protected by watchdog timer

### M7 Core (Communication)
- Reads sensor data from M4 circular buffer (FIFO order)
- Manages WiFi/Ethernet/Blues connectivity
- Publishes to MQTT broker (attempts reconnection on failure)
- Reads Modbus energy meters (optional)
- Configuration editor via serial
- Protected by watchdog timer

### Inter-Core Communication
- **1000-frame circular buffer** in shared SDRAM (83 minutes @ 5s intervals)
- **Lock-free FIFO queue**: M4 writes to head, M7 reads from tail
- **Automatic overflow handling**: Oldest frames dropped when buffer fills
- **Cache coherent**: Proper D-Cache management for dual-core safety
- **No data loss during normal operation**: Massive buffer handles extended network outages

## MQTT Message Format

### Without Modbus
```json
{
  "ver": 5,
  "signal": "-65",
  "cb": 10,
  "c1": 5,
  "c2": 0,
  "c3": 0,
  "c4": 0,
  "c5": 0,
  "c6": 0,
  "sb": 1,
  "s1": 0,
  "s2": 0,
  "s3": 0,
  "s4": 0,
  "s5": 0,
  "s6": 0,
  "a7": 2048,
  "a8": 1024
}
```

### With Modbus Energy Meter
```json
{
  "ver": 5,
  "signal": "-65",
  "cb": 10,
  ...
  "p1v1": 230.5,
  "p2v1": 231.2,
  "p3v1": 229.8,
  "p1a1": 5.4,
  "p2a1": 4.9,
  "p3a1": 5.1,
  "pf1": 0.95,
  "kWh1": 1234.5
}
```

Published to: `busroot/v1/dau/{deviceId}`

## Memory Usage

### M7 Core
- RAM: 83 KB / 523 KB (16%) - Minimal footprint!
- Flash: 487 KB / 786 KB (62%)

### M4 Core
- RAM: 43 KB / 294 KB (15%)
- Flash: 81 KB / 1 MB (8%)

### Shared SDRAM (Inter-core Buffer)
- Circular buffer: 1000 frames (~64 KB)
- Located at 0x38000000 (AHB SRAM4 domain)
- Uses entire 64 KB AHB SRAM4 region for maximum reliability
- **All buffering handled on M4 side** for simplicity and reliability

## Configuration

Configuration is stored in flash memory and persists across reboots.

## Troubleshooting

### Device won't connect to WiFi
- Check SSID and password
- Verify 2.4GHz network (5GHz not supported)
- Check signal strength
- Try increasing WiFi retry attempts in code

### Messages not appearing in MQTT
- Verify broker address and port
- Check username/password
- Ensure topic permissions
- Monitor buffer status via serial

### Missing input pulses
- Check debounce delay (50ms default)
- Verify input wiring
- Monitor serial output
- Reduce send interval if needed

### Device resets unexpectedly
- Check watchdog timeout settings
- Verify power supply stability
- Monitor error codes on LEDs (D0-D3)

## Status LEDs

The device uses LEDs D0-D3 to show status codes in binary:

| Code | Meaning |
|------|---------|
| 1 | WiFi Connecting |
| 2 | Ethernet Connecting |
| 3 | MQTT Connecting |
| 4 | Running (normal) |
| 9 | Config Load Error |
| 10 | WiFi Failed |
| 11 | MQTT Failed |
| 12 | Publish Failed |

RGB LED shows error state (red) or normal operation.

## Development

### Building
```bash
pio run                    # Build both cores
pio run -e opta_m7        # Build M7 only
pio run -e opta_m4        # Build M4 only
```

### Uploading
```bash
pio run -e opta_m7 -t upload
pio run -e opta_m4 -t upload
```

### Monitoring
```bash
pio device monitor -b 19200
```

### Clean Build
```bash
pio run -t clean
pio run
```

## Technical Specifications

- **Version**: 5
- **Platform**: STM32H747XIH6 (480MHz dual-core)
- **Framework**: Arduino (Mbed OS)
- **Send Interval**: 5 seconds (configurable)
- **Debounce Delay**: 50ms (configurable)
- **Buffer Capacity**: 1000 frames (83 minutes @ 5s intervals)
- **Serial Baud**: 19200
- **Modbus Baud**: 19200 (8N1)