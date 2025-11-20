# Busroot DAU Firmware

Data Acquisition Unit (DAU) firmware for the Arduino Opta, featuring robust input handling, MQTT communication, and 1000-frame circular buffer for maximum reliability.

Supports communication over WiFi, Ethernet and the Blues Wireless for Opta (Cellular) device.

## Features

### Input Capabilities
- 6 digital inputs (I1 to I6) with support for pulse counting and state
- Pulse counting uses falling-edge detection
- 2 analog inputs (I7 & I8) (12-bit, 0-4095 range)

### Communication
- **MQTT** support via WiFi, Ethernet, or Blues Wireless
- **Modbus RTU** support for energy meters (19200 baud)
- **Serial console** for configuration and debugging
- JSON message format

### Reliability Features
- **1000-frame circular buffer** on M4 core (83 minutes @ 5s intervals)
- **Watchdog protection** on both cores
- **Lock-free circular buffer** for safe dual-core communication
- **Race-condition-free** counter implementation
- **Configuration persistence** in flash memory
- **Automatic MQTT reconnection** on connection failure

## Quick Start

1. Visit [https://outputindustries.github.io/busroot-dau/](https://outputindustries.github.io/busroot-dau/)
2. Chose to configure via the webpage, or configure via serial.
3. Double-tap reset button to enter bootloader mode.
4. Upload firmware.


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


## MQTT Message Format

### Without Modbus
```json
{
  "ver": "v0.1.0",
  "rssi": -65,
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
  "ver": "v0.1.0",
  "rssi": -65,
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

Published to: `{prefix}/busroot/v1/dau/{deviceId}`

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