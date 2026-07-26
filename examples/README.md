# Busroot DAU Examples

This directory contains example scripts for working with Busroot DAU devices.

## MQTT Listener Example

The `mqtt-listener.js` script demonstrates how to consume data from Busroot DAU devices via MQTT.

### Installation

```bash
cd examples
npm install
```

### Usage

Run the listener with default settings (connects to localhost:1883):

```bash
node mqtt-listener.js
```

Or specify a custom MQTT broker:

```bash
node mqtt-listener.js mqtt://broker.example.com:1883
```

With authentication:

```bash
node mqtt-listener.js mqtt://username:password@broker.example.com:1883
```

### What it Does

The MQTT listener:
- Connects to an MQTT broker
- Subscribes to all Busroot DAU device topics (`+/busroot/v2/dau/+`)
- Parses and displays incoming telemetry data
- Shows key metrics including:
  - Device version and WiFi signal strength
  - Pulse counter values (inputs 1-6)
  - Input states
  - Analog readings (inputs 7-8)
  - Modbus energy meter data (if configured)

### Example Output

```
Busroot DAU MQTT Listener
========================
Connecting to: mqtt://localhost:1883
Topic pattern: +/busroot/v2/dau/+

✓ Connected to MQTT broker
✓ Subscribed to topic pattern: +/busroot/v2/dau/+

Waiting for messages...

[2026-01-28T10:30:15.123Z] Device: device-001
  Data: {
    "ver": "v0.1.0",
    "rssi": -65,
    "cb": 10,
    "c1": 5,
    "s1": 0,
    "a7": 12,
    "a8": 0
  }
  Version: v0.1.0
  WiFi Signal: -65 dBm
  Counters: c1=5
  Analog: a7=12, a8=0
```

## Building Additional Examples

You can add more examples to this directory. Common use cases include:

- **Data logging**: Store MQTT data to a database
- **Alerting**: Send notifications based on threshold values
- **Visualization**: Create real-time dashboards
- **Data transformation**: Convert to different formats for third-party systems

## Using with the Busroot Agent

The Busroot GitHub Copilot agent (`.github/agents/busroot.md`) is configured to understand and help with these examples. You can ask it to:

- Explain how the examples work
- Help modify scripts for specific use cases
- Debug issues with MQTT connectivity
- Assist with adding new examples
