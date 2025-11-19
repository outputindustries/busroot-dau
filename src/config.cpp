#include "config.h"
#include "status.h"
#include <base64.hpp>
#include <FlashIAPLimits.h>

// Config variables
CommunicationMode communicationMode = WIFI;
char deviceId[128] = "";
char wifiSsid[128] = "";
char wifiPassword[128] = "";
char mqttServer[128] = "";
int mqttPort = 1883;
char mqttUsername[128] = "";
char mqttPassword[128] = "";
char mqttClientId[128] = "";
char mqttTopicPrefix[128] = "";
int modbusDeviceCount = 0;
int modbusRegisterStyle = 0;

int p1VoltsModbusAddress = 0;
int p2VoltsModbusAddress = 0;
int p3VoltsModbusAddress = 0;
int p1AmpsModbusAddress = 0;
int p2AmpsModbusAddress = 0;
int p3AmpsModbusAddress = 0;
int pfModbusAddress = 0;
int kWhModbusAddress = 0;

bool serialOnlyMode = false;

// Config editor state machine
ConfigEditorState editorState = WAITING_INITIAL;
unsigned long editorDeadline = 0;
int currentConfigField = 0;
char inputBuffer[256] = {0};
int inputBufferIndex = 0;
bool promptShown = false;

JsonDocument configDoc;
unsigned char configToken[512] = {0};

// Flash storage
auto [flashSize, startAddress, iapSize] = getFlashIAPLimits();
FlashIAPBlockDevice blockDevice(startAddress, iapSize);

void initFlashStorage()
{
  blockDevice.init();
}

void deinitFlashStorage()
{
  blockDevice.deinit();
}

void loadConfigTokenFromMemory()
{
  blockDevice.read(configToken, 0, 512);
}

void saveConfigTokenToMemory()
{
  // Build config from current values
  JsonDocument saveDoc;
  saveDoc["v"] = VERSION;
  saveDoc["msv"] = mqttServer;
  saveDoc["mun"] = mqttUsername;
  saveDoc["mpw"] = mqttPassword;
  saveDoc["wid"] = wifiSsid;
  saveDoc["wpw"] = wifiPassword;
  saveDoc["did"] = deviceId;
  saveDoc["mci"] = mqttClientId;
  saveDoc["mtp"] = mqttTopicPrefix;
  saveDoc["mpo"] = mqttPort;
  saveDoc["mdc"] = modbusDeviceCount;

  if (communicationMode == WIFI)
  {
    saveDoc["com"] = "WIFI";
  }
  else if (communicationMode == ETHERNET)
  {
    saveDoc["com"] = "ETHERNET";
  }
  else if (communicationMode == BLUES)
  {
    saveDoc["com"] = "BLUES";
  }
  else if (communicationMode == NONE)
  {
    saveDoc["com"] = "NONE";
  }

  if (modbusDeviceCount > 0)
  {
    JsonObject mda = saveDoc["mda"].to<JsonObject>();
    mda["p1Volts"] = p1VoltsModbusAddress;
    mda["p2Volts"] = p2VoltsModbusAddress;
    mda["p3Volts"] = p3VoltsModbusAddress;
    mda["p1Amps"] = p1AmpsModbusAddress;
    mda["p2Amps"] = p2AmpsModbusAddress;
    mda["p3Amps"] = p3AmpsModbusAddress;
    mda["pf"] = pfModbusAddress;
    mda["kWh"] = kWhModbusAddress;
  }

  saveDoc["mrs"] = modbusRegisterStyle;

  // Serialize to msgpack
  unsigned char msgPack[512] = {0};
  serializeMsgPack(saveDoc, msgPack, 512);

  // Encode to base64
  memset(configToken, 0, 512);
  encode_base64(msgPack, 512, configToken);

  // Save to flash
  blockDevice.init();
  blockDevice.erase(0, 128 * 1024);
  blockDevice.program(configToken, 0, 512);
  blockDevice.deinit();

  Serial.println("Configuration saved to flash memory");
}

void applyConfigToken()
{

  unsigned char msgPack[512] = {0};
  decode_base64(configToken, 512, msgPack);
  DeserializationError err = deserializeMsgPack(configDoc, msgPack);

  if (err || !configDoc.containsKey("v"))
  {
    Serial.println("No config found or failed to decode - entering serial-only mode...");
    serialOnlyMode = true;
    showError(ERROR_CONFIG_LOAD);
    return;
  }

  strcpy(mqttServer, configDoc["msv"]);
  strcpy(mqttUsername, configDoc["mun"]);
  strcpy(mqttPassword, configDoc["mpw"]);

  if (configDoc.containsKey("com"))
  {
    const char *mode = configDoc["com"];
    if (strcmp(mode, "ETHERNET") == 0)
    {
      communicationMode = ETHERNET;
    }
    else if (strcmp(mode, "BLUES") == 0)
    {
      communicationMode = BLUES;
    }
    else if (strcmp(mode, "NONE") == 0)
    {
      communicationMode = NONE;
    }
    else
    {
      communicationMode = WIFI; // default to WIFI for any other value
    }
  }

  if (communicationMode == WIFI && configDoc.containsKey("wid"))
  {
    strcpy(wifiSsid, configDoc["wid"]);
    strcpy(wifiPassword, configDoc["wpw"]);
  }

  if (configDoc.containsKey("did"))
  {
    strcpy(deviceId, configDoc["did"]);
  }

  if (configDoc.containsKey("mci"))
  {
    strcpy(mqttClientId, configDoc["mci"]);
  }

  if (configDoc.containsKey("mtp"))
  {
    strcpy(mqttTopicPrefix, configDoc["mtp"]);
  }
  else
  {
    strcpy(mqttTopicPrefix, "");
  }

  if (configDoc.containsKey("mpo"))
  {
    mqttPort = configDoc["mpo"];
  }
  else
  {
    mqttPort = 1883;
  }

  if (configDoc.containsKey("mda"))
  {
    p1VoltsModbusAddress = configDoc["mda"]["p1Volts"];
    p2VoltsModbusAddress = configDoc["mda"]["p2Volts"];
    p3VoltsModbusAddress = configDoc["mda"]["p3Volts"];
    p1AmpsModbusAddress = configDoc["mda"]["p1Amps"];
    p2AmpsModbusAddress = configDoc["mda"]["p2Amps"];
    p3AmpsModbusAddress = configDoc["mda"]["p3Amps"];
    pfModbusAddress = configDoc["mda"]["pf"];
    kWhModbusAddress = configDoc["mda"]["kWh"];
  }

  if (configDoc.containsKey("mdc"))
  {
    modbusDeviceCount = configDoc["mdc"];
  }

  if (configDoc.containsKey("mrs"))
  {
    modbusRegisterStyle = configDoc["mrs"];
  }
}

void printConfig()
{

  Serial.print("version: ");
  Serial.println(VERSION);

  Serial.print("communicationMode: ");
  Serial.println(communicationMode == WIFI ? "WIFI" : communicationMode == ETHERNET ? "ETHERNET"
                                                  : communicationMode == BLUES      ? "BLUES"
                                                                                    : "NONE");

  Serial.print("deviceId: ");
  Serial.print(deviceId);
  Serial.println("|");

  Serial.print("wifiSsid: ");
  Serial.print(wifiSsid);
  Serial.println("|");

  Serial.print("wifiPassword: ");
  Serial.print(wifiPassword);
  Serial.println("|");

  Serial.print("mqttServer: ");
  Serial.println(mqttServer);

  Serial.print("mqttPort: ");
  Serial.println(mqttPort);

  Serial.print("mqttUsername: ");
  Serial.println(mqttUsername);

  Serial.print("mqttPassword: ");
  Serial.print(mqttPassword[0]);
  Serial.print(mqttPassword[1]);
  Serial.print(mqttPassword[2]);
  Serial.println("...");

  Serial.print("mqttClientId: ");
  Serial.println(mqttClientId);

  Serial.print("mqttTopicPrefix: ");
  Serial.println(mqttTopicPrefix);

  Serial.print("p1VoltsModbusAddress: ");
  Serial.println(p1VoltsModbusAddress);

  Serial.print("p2VoltsModbusAddress: ");
  Serial.println(p2VoltsModbusAddress);

  Serial.print("p3VoltsModbusAddress: ");
  Serial.println(p3VoltsModbusAddress);

  Serial.print("p1AmpsModbusAddress: ");
  Serial.println(p1AmpsModbusAddress);

  Serial.print("p2AmpsModbusAddress: ");
  Serial.println(p2AmpsModbusAddress);

  Serial.print("p3AmpsModbusAddress: ");
  Serial.println(p3AmpsModbusAddress);

  Serial.print("pfModbusAddress: ");
  Serial.println(pfModbusAddress);

  Serial.print("kWhModbusAddress: ");
  Serial.println(kWhModbusAddress);

  Serial.print("modbusRegisterStyle: ");
  Serial.println(modbusRegisterStyle);

  Serial.print("modbusDeviceCount: ");
  Serial.println(modbusDeviceCount);
}

void showConfigPrompt()
{
  const char *fieldNames[] = {
      "Communication Mode (WIFI/ETHERNET/BLUES/NONE)",
      "Device ID",
      "WiFi SSID",
      "WiFi Password",
      "MQTT Server",
      "MQTT Port",
      "MQTT Username",
      "MQTT Password",
      "MQTT Client ID",
      "MQTT Topic Prefix",
      "Modbus Device Count",
      "Modbus Register Style (0 or 1)"};

  if (currentConfigField >= 12)
  {
    // Done editing
    Serial.println();
    Serial.println("Configuration complete!");
    saveConfigTokenToMemory();
    editorState = RUNNING;
    promptShown = false;
    return;
  }

  if (!promptShown)
  {
    Serial.println();
    Serial.print(fieldNames[currentConfigField]);
    Serial.print(" [");

    // Show current value
    switch (currentConfigField)
    {
    case 0:
      Serial.print(communicationMode == WIFI ? "WIFI" : communicationMode == ETHERNET ? "ETHERNET"
                                                    : communicationMode == BLUES      ? "BLUES"
                                                                                      : "NONE");
      break;
    case 1:
      Serial.print(deviceId);
      break;
    case 2:
      Serial.print(wifiSsid);
      break;
    case 3:
      Serial.print(wifiPassword);
      break;
    case 4:
      Serial.print(mqttServer);
      break;
    case 5:
      Serial.print(mqttPort);
      break;
    case 6:
      Serial.print(mqttUsername);
      break;
    case 7:
      Serial.print(mqttPassword);
      break;
    case 8:
      Serial.print(mqttClientId);
      break;
    case 9:
      Serial.print(mqttTopicPrefix);
      break;
    case 10:
      Serial.print(modbusDeviceCount);
      break;
    case 11:
      Serial.print(modbusRegisterStyle);
      break;
    }

    Serial.print("]: ");
    promptShown = true;
    inputBufferIndex = 0;
    memset(inputBuffer, 0, sizeof(inputBuffer));
  }
}

void processConfigInput(char c)
{
  if (c == '\r' || c == '\n')
  {
    // Enter pressed - save value and move to next field
    Serial.println();

    // If buffer has content, update the value
    if (inputBufferIndex > 0)
    {
      inputBuffer[inputBufferIndex] = '\0';

      switch (currentConfigField)
      {
      case 0: // Communication mode
        if (strcmp(inputBuffer, "WIFI") == 0)
          communicationMode = WIFI;
        else if (strcmp(inputBuffer, "ETHERNET") == 0)
          communicationMode = ETHERNET;
        else if (strcmp(inputBuffer, "BLUES") == 0)
          communicationMode = BLUES;
        else if (strcmp(inputBuffer, "NONE") == 0)
          communicationMode = NONE;
        break;
      case 1:
        strncpy(deviceId, inputBuffer, sizeof(deviceId) - 1);
        break;
      case 2:
        strncpy(wifiSsid, inputBuffer, sizeof(wifiSsid) - 1);
        break;
      case 3:
        strncpy(wifiPassword, inputBuffer, sizeof(wifiPassword) - 1);
        break;
      case 4:
        strncpy(mqttServer, inputBuffer, sizeof(mqttServer) - 1);
        break;
      case 5:
        mqttPort = atoi(inputBuffer);
        break;
      case 6:
        strncpy(mqttUsername, inputBuffer, sizeof(mqttUsername) - 1);
        break;
      case 7:
        strncpy(mqttPassword, inputBuffer, sizeof(mqttPassword) - 1);
        break;
      case 8:
        strncpy(mqttClientId, inputBuffer, sizeof(mqttClientId) - 1);
        break;
      case 9:
        strncpy(mqttTopicPrefix, inputBuffer, sizeof(mqttTopicPrefix) - 1);
        break;
      case 10:
        modbusDeviceCount = atoi(inputBuffer);
        break;
      case 11:
        modbusRegisterStyle = atoi(inputBuffer);
        break;
      }
    }

    // Move to next field, but skip network/MQTT fields if NONE mode
    if (currentConfigField == 0 && communicationMode == NONE)
    {
      // Skip Device ID through MQTT Client ID (fields 1-8)
      currentConfigField = 9; // Jump to Modbus Device Count
    }
    else
    {
      currentConfigField++;
    }

    promptShown = false;
  }
  else if (c == 0x7F || c == 0x08)
  {
    // Backspace or delete
    if (inputBufferIndex > 0)
    {
      inputBufferIndex--;
      inputBuffer[inputBufferIndex] = '\0';
      Serial.print("\b \b"); // Erase character on screen
    }
  }
  else if (c >= 32 && c <= 126 && inputBufferIndex < sizeof(inputBuffer) - 1)
  {
    // Printable character
    inputBuffer[inputBufferIndex++] = c;
    Serial.print(c);
  }
}

void initConfigEditor()
{
  editorState = WAITING_INITIAL;
  editorDeadline = millis() + 5000; // 5 second initial wait
}

bool handleConfigEditorState()
{
  const unsigned long now = millis();

  // Config editor state machine
  if (editorState == WAITING_INITIAL)
  {
    // Wait 5 seconds, then show prompt
    if (now >= editorDeadline)
    {
      Serial.println();
      Serial.println("Press return to edit config...");
      editorState = WAITING_FOR_ENTER;
      editorDeadline = now + 10000; // Wait 10 seconds for enter
    }
    return true; // Still in editor mode
  }

  if (editorState == WAITING_FOR_ENTER)
  {
    // Cycle through LEDs D0-D3
    static int currentLed = 0;
    static unsigned long lastLedChange = 0;
    if (now - lastLedChange > 200)
    {
      // Turn off all LEDs
      digitalWrite(LED_D0, LOW);
      digitalWrite(LED_D1, LOW);
      digitalWrite(LED_D2, LOW);
      digitalWrite(LED_D3, LOW);

      // Turn on current LED
      switch (currentLed)
      {
      case 0:
        digitalWrite(LED_D0, HIGH);
        break;
      case 1:
        digitalWrite(LED_D1, HIGH);
        break;
      case 2:
        digitalWrite(LED_D2, HIGH);
        break;
      case 3:
        digitalWrite(LED_D3, HIGH);
        break;
      }

      currentLed = (currentLed + 1) % 4;
      lastLedChange = now;
    }

    // Check for enter key
    if (Serial.available())
    {
      char c = Serial.read();
      if (c == '\r' || c == '\n')
      {
        Serial.println("Entering config editor...");
        Serial.println();

        // Load existing config first
        loadConfigTokenFromMemory();
        applyConfigToken();

        editorState = EDITING_CONFIG;
        currentConfigField = 0;
        promptShown = false;
      }
    }

    // Timeout - proceed to normal operation
    if (now >= editorDeadline)
    {
      Serial.println("Timeout - starting normal operation");
      editorState = RUNNING;

      // Turn off all LEDs
      digitalWrite(LED_D0, LOW);
      digitalWrite(LED_D1, LOW);
      digitalWrite(LED_D2, LOW);
      digitalWrite(LED_D3, LOW);
    }

    return true; // Still in editor mode
  }

  if (editorState == EDITING_CONFIG)
  {

    // Show current prompt
    showConfigPrompt();

    // Process input
    if (Serial.available())
    {
      char c = Serial.read();
      processConfigInput(c);
    }

    // Stay in this state until all fields are done
    if (editorState == RUNNING)
    {
      return false; // Config editing complete
    }
    else
    {
      return true; // Still editing
    }
  }

  return false; // RUNNING state - editor done
}
