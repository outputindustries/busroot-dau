#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <ArduinoJson.h>

// Version
extern const char* VERSION;

// Communication mode enum
enum CommunicationMode
{
  WIFI,
  ETHERNET,
  BLUES,
  NONE
};

// Config editor state machine
enum ConfigEditorState
{
  WAITING_INITIAL,   // First 5 second wait
  WAITING_FOR_ENTER, // 10 second wait for enter key
  EDITING_CONFIG,    // Walking through config values
  RUNNING            // Normal operation
};

// Config variables
extern CommunicationMode communicationMode;
extern char deviceId[128];
extern char wifiSsid[128];
extern char wifiPassword[128];
extern char mqttServer[128];
extern int mqttPort;
extern char mqttUsername[128];
extern char mqttPassword[128];
extern char mqttClientId[128];
extern char mqttTopicPrefix[128];
extern int modbusDeviceCount;
extern int modbusRegisterStyle;

extern int p1VoltsModbusAddress;
extern int p2VoltsModbusAddress;
extern int p3VoltsModbusAddress;
extern int p1AmpsModbusAddress;
extern int p2AmpsModbusAddress;
extern int p3AmpsModbusAddress;
extern int pfModbusAddress;
extern int kWhModbusAddress;

extern bool serialOnlyMode;

// Config editor state
extern ConfigEditorState editorState;
extern unsigned long editorDeadline;
extern int currentConfigField;
extern char inputBuffer[256];
extern int inputBufferIndex;
extern bool promptShown;

extern JsonDocument configDoc;
extern unsigned char configToken[512];

// Function declarations
void initFlashStorage();
void deinitFlashStorage();
void loadConfigTokenFromMemory();
void saveConfigTokenToMemory();
void applyConfigToken();
void printConfig();
void showConfigPrompt();
void processConfigInput(char c);
void initConfigEditor();
bool handleConfigEditorState();

#endif // CONFIG_H
