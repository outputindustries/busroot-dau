# Busroot Opta Programmer

Web-based firmware programmer for Busroot Opta devices.

## Usage

1. Connect Opta device to computer using USB cable
2. Double-press 'Reset' button on Opta (green LED should flash slowly)
3. Click 'Connect' button to connect the browser to the Opta (Device name ends with 'Bootloader')
4. Fill in device details
5. Press 'Upload Busroot Firmware' button

## URL Query Parameters

All form fields can be pre-filled using URL query parameters. This is useful for streamlining the provisioning process.

### Available Parameters

| Parameter | Description | Example Value |
|-----------|-------------|---------------|
| `device-id` | Unique device identifier | `opta-001` |
| `communication-mode` | Communication method | `WIFI`, `ETHERNET`, `BLUES`, or `NONE` |
| `wifi-ssid` | WiFi network name | `MyNetwork` |
| `wifi-password` | WiFi password | `MyPassword123` |
| `mqtt-server` | MQTT broker address | `eu-west-2-2.busroot.com` |
| `mqtt-port` | MQTT broker port | `1885` |
| `mqtt-username` | MQTT username | `device-user` |
| `mqtt-password` | MQTT password | `secretpass` |
| `mqtt-client-id` | MQTT client ID | `opta-001/12345` |
| `modbus-device-name` | Modbus device model | `RS PRO - 236-929X` or `Carlo Gavazzi - EM210` |
| `modbus-device-count` | Number of Modbus devices | `1` to `6` |

### Example URLs

#### WiFi Configuration

```
http://localhost:3000/?device-id=opta-warehouse-01&communication-mode=WIFI&wifi-ssid=WarehouseNetwork&wifi-password=SecurePass123&mqtt-server=eu-west-2-2.busroot.com&mqtt-port=1885&mqtt-username=opta-warehouse-01&mqtt-password=mqttpass123&modbus-device-name=RS%20PRO%20-%20236-929X&modbus-device-count=1
```

#### Ethernet Configuration

```
http://localhost:3000/?device-id=opta-factory-02&communication-mode=ETHERNET&mqtt-server=mqtt.example.com&mqtt-port=1883&mqtt-username=factory-device&mqtt-password=devicepass456&modbus-device-name=Carlo%20Gavazzi%20-%20EM210&modbus-device-count=2
```

#### Blues Wireless Configuration

```
http://localhost:3000/?device-id=opta-remote-03&communication-mode=BLUES&mqtt-server=eu-west-2-2.busroot.com&mqtt-port=1885&mqtt-username=opta-remote-03&mqtt-password=remotepass789&modbus-device-name=RS%20PRO%20-%20236-929X&modbus-device-count=1
```

#### No Communication Mode (Local Only)

```
http://localhost:3000/?device-id=opta-standalone&communication-mode=NONE&modbus-device-name=Carlo%20Gavazzi%20-%20EM210&modbus-device-count=3
```

### Notes

- URL encode special characters (spaces become `%20`, etc.)
- Parameters are optional - any missing values will use defaults or remain empty
- MQTT Username defaults to Device ID if not specified
- MQTT Server defaults to `eu-west-2-2.busroot.com` if not specified
- MQTT Port defaults to `1885` if not specified
- WiFi settings are only shown when Communication Mode is `WIFI`
- MQTT settings are hidden when Communication Mode is `NONE`

## Development

### Install Dependencies

```bash
npm install
```

### Run Development Server

```bash
npm run dev
```

### Build for Production

```bash
npm run build
```

## Supported Modbus Devices

- **RS PRO - 236-929X** (Register Style 0)
- **Carlo Gavazzi - EM210** (Register Style 1)

## Browser Compatibility

This tool requires WebUSB support. Use Google Chrome or Microsoft Edge.
