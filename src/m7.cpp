#include <Arduino.h>
#include <Notecard.h>
#include <WiFi.h>
#include <SPI.h>
#include <PortentaEthernet.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <Watchdog.h>
#include <ArduinoModbus.h>
#include <ArduinoRS485.h>
#include <ArduinoJson.h>
#include "config.h"
#include "status.h"
#include "data_frame.h"
#include "SDRAM.h"

// VERSION is injected at compile-time from build flags
#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "dev-unknown"
#endif
const char* VERSION = FIRMWARE_VERSION;

// Message buffer for failed MQTT publishes
// Optimized sizes based on actual message content:
// - Topic: "busroot/v1/dau/XXXXXXXXXXXX" = max 29 bytes -> allocated 32
// - Message without Modbus: ~250 bytes max
// - Message with Modbus: ~390 bytes max -> allocated 512 for safety
struct BufferedMessage
{
  char topic[32];    // Was 128, actual max ~29 bytes
  char message[512]; // Was 2056, actual max ~390 bytes
  unsigned long timestamp;
  bool occupied;
};

// Maximized buffer: 600 messages = 50 minutes at 5-second intervals
// Per message: 32 + 512 + 4 + 1 = 549 bytes
// Total buffer RAM: ~329 KB
// Total expected RAM usage: ~412 KB / 523 KB (78.7%)
const int MESSAGE_BUFFER_SIZE = 600;
BufferedMessage messageBuffer[MESSAGE_BUFFER_SIZE];
int bufferHead = 0;  // Next position to write
int bufferTail = 0;  // Next position to read
int bufferCount = 0; // Number of messages in buffer

/*
 * Config Token Key Mapping:
 * v   = version
 * msv = mqttServer
 * mun = mqttUsername
 * mpw = mqttPassword
 * wid = wifiSsid
 * wpw = wifiPassword
 * did = deviceId
 * mci = mqttClientId
 * mpo = mqttPort
 * mdc = modbusDeviceCount
 * com = communicationMode (ETHERNET, WIFI, BLUES)
 */

Notecard notecard;

bool setupComplete = false;

WiFiClient wifiClient;
EthernetClient ethClient;
PubSubClient *mqttClient = nullptr;

unsigned int wifiAttempts = 0;
unsigned int mqttAttempts = 0;

constexpr auto modbus_baudrate{19200};
constexpr auto wordlen{9.6f}; // try also with 10.0f
constexpr auto modbus_bitduration{1.f / modbus_baudrate};
constexpr auto modbus_preDelayBR{modbus_bitduration * wordlen * 3.5f * 1e6};
constexpr auto modbus_postDelayBR{modbus_bitduration * wordlen * 3.5f * 1e6};

void printEncryptionType(int thisType)
{
  // read the encryption type and print out the name:
  switch (thisType)
  {
  case ENC_TYPE_WEP:
    Serial.println("WEP");
    break;
  case ENC_TYPE_TKIP:
    Serial.println("WPA");
    break;
  case ENC_TYPE_CCMP:
    Serial.println("WPA2");
    break;
  case ENC_TYPE_NONE:
    Serial.println("None");
    break;
  case ENC_TYPE_AUTO:
    Serial.println("Auto");
    break;
  }
}

void listNetworks()
{
  // scan for nearby networks:
  int numSsid = WiFi.scanNetworks();
  if (numSsid == -1)
  {
    Serial.println("Couldn't get a wifi connection");
    while (true)
      ;
  }

  // print the list of networks seen:
  Serial.print("number of available networks:");
  Serial.println(numSsid);

  // print the network number and name for each network found:
  for (int thisNet = 0; thisNet < numSsid; thisNet++)
  {
    Serial.print(thisNet);
    Serial.print(") ");
    Serial.print(WiFi.SSID(thisNet));
    Serial.print("\tSignal: ");
    Serial.print(WiFi.RSSI(thisNet));
    Serial.print(" dBm");
    Serial.print("\tEncryption: ");
    printEncryptionType(WiFi.encryptionType(thisNet));
  }
}

void setWifiMacAddress()
{
  if (strlen(deviceId) > 0)
  {
    return;
  }

  byte mac[6];
  WiFi.macAddress(mac);
  sprintf(deviceId, "%02X%02X%02X%02X%02X%02X", mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);

  Serial.print("WiFi MAC: ");
  Serial.println(deviceId);
}

void setEthernetMacAddress()
{
  if (strlen(deviceId) > 0)
  {
    return;
  }

  byte mac[6];
  Ethernet.macAddress(mac);
  sprintf(deviceId, "%02X%02X%02X%02X%02X%02X", mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);

  Serial.print("Ethernet MAC: ");
  Serial.println(deviceId);
}

bool addToBuffer(const char *topic, const char *message)
{
  if (bufferCount >= MESSAGE_BUFFER_SIZE)
  {
    Serial.println("Buffer full! Dropping oldest message");
    // Drop oldest message to make room
    bufferTail = (bufferTail + 1) % MESSAGE_BUFFER_SIZE;
    bufferCount--;
  }

  // Add new message at head
  strncpy(messageBuffer[bufferHead].topic, topic, sizeof(messageBuffer[bufferHead].topic) - 1);
  messageBuffer[bufferHead].topic[sizeof(messageBuffer[bufferHead].topic) - 1] = '\0';

  strncpy(messageBuffer[bufferHead].message, message, sizeof(messageBuffer[bufferHead].message) - 1);
  messageBuffer[bufferHead].message[sizeof(messageBuffer[bufferHead].message) - 1] = '\0';

  messageBuffer[bufferHead].timestamp = millis();
  messageBuffer[bufferHead].occupied = true;

  bufferHead = (bufferHead + 1) % MESSAGE_BUFFER_SIZE;
  bufferCount++;

  Serial.print("Added to buffer. Buffer count: ");
  Serial.println(bufferCount);

  return true;
}

bool sendFromBuffer()
{
  if (bufferCount == 0)
  {
    return false; // No messages to send
  }

  if (serialOnlyMode)
  {
    // In serial-only mode, just discard buffered messages
    bufferTail = (bufferTail + 1) % MESSAGE_BUFFER_SIZE;
    bufferCount--;
    return true;
  }

  if (!mqttClient || !mqttClient->connected())
  {
    return false; // Can't send, MQTT not ready
  }

  // Try to send oldest message
  BufferedMessage *msg = &messageBuffer[bufferTail];

  if (!msg->occupied)
  {
    // Skip empty slot
    bufferTail = (bufferTail + 1) % MESSAGE_BUFFER_SIZE;
    bufferCount--;
    return false;
  }

  Serial.print("Attempting to send buffered message: ");
  Serial.println(msg->topic);

  bool success = mqttClient->publish(msg->topic, msg->message);

  if (success)
  {
    // Success! Remove from buffer
    msg->occupied = false;
    bufferTail = (bufferTail + 1) % MESSAGE_BUFFER_SIZE;
    bufferCount--;

    Serial.print("Sent buffered message. Remaining: ");
    Serial.println(bufferCount);
    return true;
  }
  else
  {
    Serial.println("Failed to send buffered message, will retry later");
    return false;
  }
}

float getModbusRegister(int id, int address)
{
  if (ModbusRTUClient.requestFrom(id, INPUT_REGISTERS, address, 2) == 0)
  {
    Serial.print("failed! ");
    Serial.println(ModbusRTUClient.lastError());

    return -1;
  }
  else
  {
    uint16_t reg1 = ModbusRTUClient.read();
    uint16_t reg2 = ModbusRTUClient.read();

    // 32 bit IEEE754 floating point numbers (Used by RS-Pro devices)
    if (modbusRegisterStyle == 0)
    {
      uint32_t regs = (reg1 << 16) | reg2;

      float v = *(float *)&regs;

      return v;
    }
    // MSB->LSB, LSW-> MSW (Used by Carlo Gavazzi devices)
    else if (modbusRegisterStyle == 1)
    {
      uint32_t regs = (reg2 << 16) | reg1;

      return (float)regs;
    }
  }

  return -1; // Default return for failed request
}

void setupWifi()
{

  mbed::Watchdog::get_instance().kick();

  WiFi.disconnect();
  delay(5000);

  setDeviceState(STATE_WIFI_CONNECTING);

  mbed::Watchdog::get_instance().kick();

  Serial.println();
  Serial.println("Scanning WiFi networks...");
  listNetworks();

  mbed::Watchdog::get_instance().kick();

  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifiSsid);

  int status;

  // Poll WiFi status with frequent watchdog kicks
  while (status != WL_CONNECTED)
  {
    Serial.print(".");

    if (wifiAttempts > 10)
    {
      Serial.println();
      Serial.print("WiFi status: ");
      Serial.println(WiFi.status());
      Serial.println("WiFi connection failed after 10 attempts");
      showError(ERROR_WIFI_FAILED);
      HAL_NVIC_SystemReset();
    }

    // Initiate WiFi connection (non-blocking)
    if (strcmp(wifiPassword, "") == 0)
    {
      status = WiFi.begin(wifiSsid);
    }
    else
    {
      status = WiFi.begin(wifiSsid, wifiPassword);
    }

    // Kick watchdog frequently to prevent timeout during connection
    mbed::Watchdog::get_instance().kick();

    delay(3000); // Short delay for faster status checking

    wifiAttempts++;
  }

  setWifiMacAddress();

  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect()
{
  if (!mqttClient)
    return;

  // If using WiFi, check and reconnect WiFi first if needed
  if (communicationMode == WIFI)
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("WiFi disconnected, attempting to reconnect...");
      setDeviceState(STATE_WIFI_CONNECTING);

      // Reset WiFi attempts counter for reconnection
      unsigned int wifiReconnectAttempts = 0;

      while (WiFi.status() != WL_CONNECTED && wifiReconnectAttempts < 10)
      {
        Serial.print(".");

        int status;
        if (strcmp(wifiPassword, "") == 0)
        {
          status = WiFi.begin(wifiSsid);
        }
        else
        {
          status = WiFi.begin(wifiSsid, wifiPassword);
        }

        mbed::Watchdog::get_instance().kick();
        delay(3000);
        wifiReconnectAttempts++;
      }

      if (WiFi.status() == WL_CONNECTED)
      {
        Serial.println();
        Serial.println("WiFi reconnected");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        // Reset MQTT attempts counter after successful WiFi reconnection
        mqttAttempts = 0;
      }
      else
      {
        Serial.println();
        Serial.println("WiFi reconnection failed");
        showError(ERROR_WIFI_FAILED);
        HAL_NVIC_SystemReset();
      }
    }
  }

  setDeviceState(STATE_MQTT_CONNECTING);

  while (!mqttClient->connected())
  {
    Serial.print("Attempting MQTT connection...");

    if (mqttClient->connect(mqttClientId, mqttUsername, mqttPassword))
    {
      Serial.println("connected");
      break;
    }
    else
    {
      Serial.print("failed. rc=");
      Serial.print(mqttClient->state());
      Serial.println(" trying again...");
    }

    mqttAttempts++;
    if (mqttAttempts > 10)
    {
      showError(ERROR_MQTT_FAILED);
      HAL_NVIC_SystemReset();
    }

    // Wait 2 seconds between attempts with watchdog kicks every 500ms
    for (int i = 0; i < 4; i++)
    {
      mbed::Watchdog::get_instance().kick();
      delay(500);
    }
  }
}

void setupNetworking()
{
  // MODBUS
  RS485.setDelays(modbus_preDelayBR, modbus_postDelayBR);
  if (!ModbusRTUClient.begin(modbus_baudrate, SERIAL_8N1))
  {
    Serial.println("Failed to start Modbus RTU Client!");
  }
  ModbusRTUClient.setTimeout(500);

  mbed::Watchdog::get_instance().kick();

  if (communicationMode == ETHERNET)
  {
    setDeviceState(STATE_ETHERNET_CONNECTING);
    Serial.println("Trying Ethernet:");
    while (Ethernet.begin(nullptr, 10000, 4000) == 0)
    {
      Serial.println("Failed to configure Ethernet using DHCP...");
      if (Ethernet.linkStatus() == LinkOFF)
      {
        Serial.println("Ethernet cable is not connected...");
      }
      // Kick watchdog to prevent timeout during DHCP attempts
      mbed::Watchdog::get_instance().kick();
      delay(1000);
    }

    Serial.print("Connected via Ethernet: ");
    Serial.println(Ethernet.localIP());

    ethClient.setTimeout(5000); // 5 second timeout
    mqttClient = new PubSubClient(ethClient);
    setEthernetMacAddress();
  }
  else if (communicationMode == WIFI)
  {
    setupWifi();
    wifiClient.setTimeout(5000); // 5 second timeout
    mqttClient = new PubSubClient(wifiClient);
  }
  else if (communicationMode == BLUES)
  {
    notecard.setDebugOutputStream(Serial);
    notecard.begin();
  }
  else if (communicationMode == NONE)
  {
    Serial.println("Communication mode: NONE - Serial output only");
  }

  mbed::Watchdog::get_instance().kick();

  if (mqttClient)
  {
    Serial.println("=== MQTT Configuration ===");
    if (communicationMode == ETHERNET) {
      Serial.print("Network: Ethernet - IP: ");
      Serial.println(Ethernet.localIP());
    } else if (communicationMode == WIFI) {
      Serial.print("Network: WiFi - IP: ");
      Serial.println(WiFi.localIP());
      Serial.print("RSSI: ");
      Serial.println(WiFi.RSSI());
    }
    Serial.print("Server: ");
    Serial.println(mqttServer);
    Serial.print("Port: ");
    Serial.println(mqttPort);
    Serial.print("Client ID: ");
    Serial.println(mqttClientId);
    Serial.print("Username: ");
    Serial.println(mqttUsername);
    Serial.print("Password: ");
    Serial.println(mqttPassword);
    Serial.print("Buffer Size: ");
    Serial.println(2560);
    Serial.print("Keep Alive: ");
    Serial.println(15);
    Serial.println("========================");

    mqttClient->setServer(mqttServer, mqttPort);
    mqttClient->setBufferSize(2560); // Increased from 2056 to add safety margin
    mqttClient->setKeepAlive(15); // Keep connection alive with 15 second keepalive
    reconnect();
  }
}

void setup()
{
  mbed::Watchdog::get_instance().start();

  pinMode(LEDR, OUTPUT);
  pinMode(LEDG, OUTPUT);
  pinMode(LEDB, OUTPUT);
  pinMode(LED_D0, OUTPUT);
  pinMode(LED_D1, OUTPUT);
  pinMode(LED_D2, OUTPUT);
  pinMode(LED_D3, OUTPUT);

  digitalWrite(LEDR, 0);
  digitalWrite(LEDG, 0);
  digitalWrite(LEDB, 0);
  digitalWrite(LED_D0, 0);
  digitalWrite(LED_D1, 0);
  digitalWrite(LED_D2, 0);
  digitalWrite(LED_D3, 0);

  Serial.begin(19200); // The USB serial connection

  for (const auto timeout = millis() + 2500; !Serial && millis() < timeout; delay(250))
    ;

  // Boot M4 core after basic initialization
  bootM4();

  initFlashStorage();

  // Initialize config editor state machine
  initConfigEditor();
}

bool attemptPublish(const char *topic, const char *message)
{
  if ((communicationMode == WIFI || communicationMode == ETHERNET) && mqttClient)
  {
    // If MQTT is down, try reconnecting
    if (!mqttClient->connected())
    {
      reconnect();
    }

    // Attempt to publish
    if (mqttClient->connected())
    {
      bool success = mqttClient->publish(topic, message);
      if (!success)
      {
        Serial.println("MQTT publish failed");

        // Set running state
        setDeviceState(ERROR_PUBLISH_FAILED);
      }
      return success;
    }
    return false;
  }
  else if (communicationMode == BLUES)
  {
    // Create a JSON request object for the Notecard
    J *req = notecard.newRequest("note.add");
    if (req != NULL)
    {
      JAddStringToObject(req, "file", "data.qo");
      JAddBoolToObject(req, "sync", true);

      // Add the message as body
      J *body = JCreateObject();
      if (body != NULL)
      {
        JAddStringToObject(body, "topic", topic);
        JAddStringToObject(body, "message", message);
        JAddItemToObject(req, "body", body);
      }

      return notecard.sendRequest(req);
    }
    return false;
  }

  return true; // Serial-only mode always succeeds
}

void sendMessage(const char *topic, const char *message)
{
  setDeviceState(STATE_PUBLISHING);
  // Always output to serial
  Serial.println();
  Serial.println(topic);
  Serial.println(message);
  Serial.println();

  if (serialOnlyMode)
  {
    delay(500);
    return;
  }
  else
  {

    // Attempt to publish the new message
    if (attemptPublish(topic, message))
    {
      // Connection is working - try to flush buffer
      if (bufferCount > 0)
      {
        Serial.print("Flushing buffer (");
        Serial.print(bufferCount);
        Serial.println(" messages)...");

        // Try to send up to 5 buffered messages while connection is good
        for (int i = 0; i < 5 && bufferCount > 0; i++)
        {
          if (!sendFromBuffer())
          {
            break; // Stop if a send fails
          }
          mbed::Watchdog::get_instance().kick();
        }
      }
    }
    else
    {
      // Failed to send - add to buffer for retry
      Serial.println("Failed to send message - adding to buffer");
      
      // Temporarily disable buffer.
      //addToBuffer(topic, message);
    }
  }

  setDeviceState(STATE_RUNNING);
}

void loop()
{
  // Config editor state machine
  mbed::Watchdog::get_instance().kick();
  if (handleConfigEditorState())
  {
    return; // Still in config editor mode
  }

  if (!setupComplete)
  {
    loadConfigTokenFromMemory();
    deinitFlashStorage();
    applyConfigToken();

    if (!serialOnlyMode)
    {
      printConfig();
      setupNetworking();

      if (strlen(mqttClientId) == 0)
      {
        strcpy(mqttClientId, deviceId);
      }
    }
    else
    {
      Serial.println("Running in serial-only mode - network disabled");
    }

    setupComplete = true;

    // Set running state
    setDeviceState(STATE_RUNNING);
  }

  // void receiveDataFromM4()
  // Invalidate cache before reading flag to ensure we see fresh data
  invalidateSharedMemoryCache();

  if (*send_frame_ready_flag == 1)
  {
    // Invalidate cache again to ensure we read fresh data
    invalidateSharedMemoryCache();

    // Copy data from volatile SDRAM (field by field to avoid volatile assignment issues)
    DATA_FRAME_SEND dataFromM4;
    dataFromM4.userButtonCount = data_frame_send_sdram->userButtonCount;
    dataFromM4.input1Count = data_frame_send_sdram->input1Count;
    dataFromM4.input2Count = data_frame_send_sdram->input2Count;
    dataFromM4.input3Count = data_frame_send_sdram->input3Count;
    dataFromM4.input4Count = data_frame_send_sdram->input4Count;
    dataFromM4.input5Count = data_frame_send_sdram->input5Count;
    dataFromM4.input6Count = data_frame_send_sdram->input6Count;
    dataFromM4.userButtonState = data_frame_send_sdram->userButtonState;
    dataFromM4.input1State = data_frame_send_sdram->input1State;
    dataFromM4.input2State = data_frame_send_sdram->input2State;
    dataFromM4.input3State = data_frame_send_sdram->input3State;
    dataFromM4.input4State = data_frame_send_sdram->input4State;
    dataFromM4.input5State = data_frame_send_sdram->input5State;
    dataFromM4.input6State = data_frame_send_sdram->input6State;
    dataFromM4.input7Analog = data_frame_send_sdram->input7Analog;
    dataFromM4.input8Analog = data_frame_send_sdram->input8Analog;

    *send_frame_ready_flag = 0; // Clear flag to let M4 know data has been read

    // Clean cache to make flag clear visible to M4
    cleanSharedMemoryCache();

    // Get Wifi strength
    char rssi[8] = "-1";
    if (communicationMode == WIFI)
    {
      String(WiFi.RSSI()).toCharArray(rssi, 8);
    }

    char topic[128] = {0};
    char message[2056] = {0};

    digitalWrite(LEDB, 1);
    snprintf(topic, sizeof(topic), "busroot/v1/dau/%s", deviceId);

    // MODBUS
    if (modbusDeviceCount == 0)
    {
      snprintf(message, sizeof(message), "{\"v\":\"%s\",\"signal\":\"%s\",\"cb\":%u,\"c1\":%u,\"c2\":%u,\"c3\":%u,\"c4\":%u,\"c5\":%u,\"c6\":%u,\"sb\":%u,\"s1\":%u,\"s2\":%u,\"s3\":%u,\"s4\":%u,\"s5\":%u,\"s6\":%u,\"a7\":%u,\"a8\":%u}",
               VERSION,
               rssi,
               dataFromM4.userButtonCount,
               dataFromM4.input1Count,
               dataFromM4.input2Count,
               dataFromM4.input3Count,
               dataFromM4.input4Count,
               dataFromM4.input5Count,
               dataFromM4.input6Count,
               dataFromM4.userButtonState,
               dataFromM4.input1State,
               dataFromM4.input2State,
               dataFromM4.input3State,
               dataFromM4.input4State,
               dataFromM4.input5State,
               dataFromM4.input6State,
               dataFromM4.input7Analog,
               dataFromM4.input8Analog);
    }
    else
    {
      // Temporarily remove support for multiple Modbus devices
      const int i = 0;

      // When changing address, need to perform a dummy request
      // Not sure why this happens, but first request after address change always fails.
      getModbusRegister(i + 1, 0x00);

      const float p1Volts = getModbusRegister(i + 1, p1VoltsModbusAddress);
      const float p2Volts = getModbusRegister(i + 1, p2VoltsModbusAddress);
      const float p3Volts = getModbusRegister(i + 1, p3VoltsModbusAddress);
      const float p1Amps = getModbusRegister(i + 1, p1AmpsModbusAddress);
      const float p2Amps = getModbusRegister(i + 1, p2AmpsModbusAddress);
      const float p3Amps = getModbusRegister(i + 1, p3AmpsModbusAddress);
      const float pf = getModbusRegister(i + 1, pfModbusAddress);
      const float kWh = getModbusRegister(i + 1, kWhModbusAddress);

      snprintf(message, sizeof(message), "{\"v\":\"%s\",\"signal\":\"%s\",\"cb\":%u,\"c1\":%u,\"c2\":%u,\"c3\":%u,\"c4\":%u,\"c5\":%u,\"c6\":%u,\"sb\":%u,\"s1\":%u,\"s2\":%u,\"s3\":%u,\"s4\":%u,\"s5\":%u,\"s6\":%u,\"a7\":%u,\"a8\":%u,\"p1v%u\":%.4f,\"p2v%u\":%.4f,\"p3v%u\":%.4f,\"p1a%u\":%.4f,\"p2a%u\":%.4f,\"p3a%u\":%.4f,\"pf%u\":%.4f,\"kWh%u\":%.4f}",
               VERSION,
               rssi,
               dataFromM4.userButtonCount,
               dataFromM4.input1Count,
               dataFromM4.input2Count,
               dataFromM4.input3Count,
               dataFromM4.input4Count,
               dataFromM4.input5Count,
               dataFromM4.input6Count,
               dataFromM4.userButtonState,
               dataFromM4.input1State,
               dataFromM4.input2State,
               dataFromM4.input3State,
               dataFromM4.input4State,
               dataFromM4.input5State,
               dataFromM4.input6State,
               dataFromM4.input7Analog,
               dataFromM4.input8Analog,
               i + 1, p1Volts,
               i + 1, p2Volts,
               i + 1, p3Volts,
               i + 1, p1Amps,
               i + 1, p2Amps,
               i + 1, p3Amps,
               i + 1, pf,
               i + 1, kWh);

      mbed::Watchdog::get_instance().kick();
    }

    // Remove unnecessary trailing zeros.
    String messageString = String(message);
    messageString.replace(".0000", "");
    messageString.toCharArray(message, 2056);

    sendMessage(topic, message);

    memset(topic, 0, sizeof(topic));
    memset(message, 0, sizeof(message));

    digitalWrite(LEDB, 0);
  }

  if (mqttClient)
  {
    mqttClient->loop();
  }

  // Try to send buffered messages if any exist
  // Attempt to send one message per loop iteration to avoid blocking
  if (bufferCount > 0)
  {
    sendFromBuffer();
  }

  mbed::Watchdog::get_instance().kick();
}