# Busroot DAU Firmware

Industrial-grade Data Acquisition Unit (DAU) firmware for Arduino Opta, featuring dual-core architecture with robust input handling, MQTT communication, and extensive message buffering.

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
- ✅ **600-message buffer** (50 minutes @ 5s intervals)
- ✅ **Watchdog protection** on both cores
- ✅ **Automatic retry** for failed messages
- ✅ **Volatile memory** for safe multi-core communication
- ✅ **Race-condition-free** counter implementation
- ✅ **Configuration persistence** in flash memory

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

## Web Interface

A complete web-based firmware distribution interface is included:

```bash
cd web
python3 -m http.server 8000
```

Open `http://localhost:8000` for:
- Firmware downloads
- Installation instructions
- Device configuration guide
- Technical specifications

See [web/README.md](web/README.md) for hosting options.

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
- Sends data to M7 every 5 seconds via SDRAM
- Protected by watchdog timer

### M7 Core (Communication)
- Receives sensor data from M4
- Manages WiFi/Ethernet/Blues connectivity
- Publishes to MQTT broker
- Reads Modbus energy meters (optional)
- Buffers failed messages (600 messages)
- Configuration editor via serial
- Protected by watchdog timer

### Inter-Core Communication
- Shared SDRAM memory region
- Simple flag-based handshake protocol
- Volatile pointers for memory safety
- No race conditions or data loss

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
- RAM: 414 KB / 523 KB (79%)
- Flash: 488 KB / 786 KB (62%)
- Buffer: 600 messages (329 KB)

### M4 Core
- RAM: 43 KB / 294 KB (15%)
- Flash: 84 KB / 1 MB (8%)

## Configuration

Configuration is stored in flash memory and persists across reboots.

### Example Configuration
```
Communication Mode: WIFI
Device ID: A1B2C3D4E5F6
WiFi SSID: MyNetwork
WiFi Password: ********
MQTT Server: mqtt.example.com
MQTT Port: 1883
MQTT Username: device01
MQTT Password: ********
MQTT Client ID: opta-a1b2c3
Modbus Device Count: 1
```

### Serial-Only Mode
If no configuration is found, device enters serial-only mode:
- Prints all data to serial console
- No network communication
- Useful for debugging

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
- **Buffer Capacity**: 600 messages (50 minutes)
- **Serial Baud**: 19200
- **Modbus Baud**: 19200 (8N1)

## License

Copyright (c) 2025 Busroot

## Support

For issues and questions, please use the project issue tracker or contact support.
