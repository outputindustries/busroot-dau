#include "status.h"
#include <Watchdog.h>

// Convert state code to human-readable name
const char *getStateName(uint8_t state)
{
  switch (state)
  {
  case STATE_WIFI_CONNECTING:
    return "WIFI_CONNECTING";
  case STATE_ETHERNET_CONNECTING:
    return "ETHERNET_CONNECTING";
  case STATE_MQTT_CONNECTING:
    return "MQTT_CONNECTING";
  case STATE_RUNNING:
    return "RUNNING";
  case STATE_READING_M4:
    return "READING_M4";
  case STATE_DECODING_PROTOBUF:
    return "DECODING_PROTOBUF";
  case STATE_READING_MODBUS:
    return "READING_MODBUS";
  case STATE_PUBLISHING:
    return "PUBLISHING";
  case ERROR_CONFIG_LOAD:
    return "ERROR_CONFIG_LOAD";
  case ERROR_WIFI_FAILED:
    return "ERROR_WIFI_FAILED";
  case ERROR_MQTT_FAILED:
    return "ERROR_MQTT_FAILED";
  case ERROR_PUBLISH_FAILED:
    return "ERROR_PUBLISH_FAILED";
  case ERROR_DECODE_FAILED:
    return "ERROR_DECODE_FAILED";
  case ERROR_BUFFER_OVERFLOW:
    return "ERROR_BUFFER_OVERFLOW";
  case ERROR_UNKNOWN:
    return "ERROR_UNKNOWN";
  default:
    return "UNKNOWN";
  }
}

// Display state code on LED_D0-D3 in binary (bit 0-3)
void setDeviceState(uint8_t state)
{

  Serial.print("STATE: ");
  Serial.println(getStateName(state));

  digitalWrite(LED_D0, (state & 0x08) ? HIGH : LOW); // Bit 0
  digitalWrite(LED_D1, (state & 0x04) ? HIGH : LOW); // Bit 1
  digitalWrite(LED_D2, (state & 0x02) ? HIGH : LOW); // Bit 2
  digitalWrite(LED_D3, (state & 0x01) ? HIGH : LOW); // Bit 3
}

// Show error: RGB LED red, display error code for 10 seconds, then restart
void showError(uint8_t errorCode)
{
  // Set RGB LED to red
  digitalWrite(LEDR, HIGH);
  digitalWrite(LEDG, LOW);

  // Display error code on D0-D3
  setDeviceState(errorCode);

  // Log to serial
  Serial.print("FATAL ERROR: ");
  Serial.print(getStateName(errorCode));
  Serial.print(" (");
  Serial.print(errorCode);
  Serial.println(")");

  // Wait 5 seconds while kicking watchdog
  for (int i = 0; i < 5; i++)
  {
    mbed::Watchdog::get_instance().kick();
    delay(1000);
  }

  digitalWrite(LEDR, LOW);
  digitalWrite(LEDG, LOW);
}
