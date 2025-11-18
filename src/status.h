#ifndef STATUS_H
#define STATUS_H

#include <Arduino.h>

// Device state and error codes (0-15) displayed on LED_D0-D3 in binary
enum DeviceState
{
  STATE_WIFI_CONNECTING = 1,     // Connecting to WiFi
  STATE_ETHERNET_CONNECTING = 2, // Connecting to Ethernet
  STATE_MQTT_CONNECTING = 3,     // Connecting to MQTT broker
  STATE_RUNNING = 4,             // Normal operation
  STATE_READING_M4 = 5,          // Reading data from M4 core
  STATE_DECODING_PROTOBUF = 6,   // Decoding protobuf message
  STATE_READING_MODBUS = 7,      // Reading Modbus devices
  STATE_PUBLISHING = 8,          // Publishing MQTT message
  ERROR_CONFIG_LOAD = 9,         // Config load/decode failed
  ERROR_WIFI_FAILED = 10,        // WiFi connection failed
  ERROR_MQTT_FAILED = 11,        // MQTT connection failed
  ERROR_PUBLISH_FAILED = 12,     // MQTT connection failed
  ERROR_DECODE_FAILED = 13,      // Protobuf decode failed
  ERROR_BUFFER_OVERFLOW = 14,    // RPC buffer overflow
  ERROR_UNKNOWN = 15             // Catch-all error
};

// Function declarations
void setDeviceState(uint8_t state);
void showError(uint8_t errorCode);

#endif // STATUS_H
