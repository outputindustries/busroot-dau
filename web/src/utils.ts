import { encode, decode } from "./messagepack";
import { ModbusDeviceAddresses } from "./modbus-devices";

const VERSION = 3;

interface DauConfig {
  v: number;
  did: string;
  com: "WIFI" | "ETHERNET" | "BLUES" | "NONE";
  wid?: string;
  wpw?: string;
  msv: string;
  mpo: number;
  mun: string;
  mpw: string;
  mci: string;
  mda: ModbusDeviceAddresses;
  mdc: number;
  mrs: 0 | 1;
}

interface DauTokenInputs {
  deviceId: string;
  communicationMode: "WIFI" | "ETHERNET" | "BLUES" | "NONE";
  mqttServer?: string;
  mqttPort?: number;
  mqttUsername?: string;
  mqttPassword?: string;
  mqttClientId?: string;
  wifiSsid?: string;
  wifiPassword?: string;
  modbusDeviceAddresses: ModbusDeviceAddresses
  modbusDeviceCount: number | string;
  modbusRegisterStyle: 0 | 1;
}

export function generateDauConfig(tokenInputs: DauTokenInputs) {
  const config: DauConfig = {
    v: VERSION,
    did: tokenInputs.deviceId,
    com: tokenInputs.communicationMode,
    msv: tokenInputs.mqttServer || "eu-west-2-2.busroot.com",
    mpo: tokenInputs.mqttPort || 1885,
    mun: tokenInputs.mqttUsername || tokenInputs.deviceId,
    mpw: tokenInputs.mqttPassword || "",
    mci: tokenInputs.mqttClientId || (tokenInputs.deviceId + "/" + Date.now().toString().substring(5)), // everything after slash is ignored by Busroot, but overall Client ID needs to be unique for MQTT
    mda: tokenInputs.modbusDeviceAddresses,
    mdc: tokenInputs.modbusDeviceAddresses ? parseInt(tokenInputs.modbusDeviceCount.toString()) : 0,
    mrs: tokenInputs.modbusRegisterStyle || 0
  };

  if (tokenInputs.communicationMode === "WIFI") {
    config.wid = tokenInputs.wifiSsid;
    config.wpw = tokenInputs.wifiPassword;
  }

  return config;
}

export function generateDauToken(dauConfig: DauConfig) {
  const msgPack = encode(dauConfig);

  let inString = "";
  for (let i = 0; i < msgPack.length; i++) {
    inString += String.fromCharCode(msgPack[i]);
  }

  const b64String = btoa(inString);

  // DEBUG: Reverse process to test

  const msgPack2String = atob(b64String);

  const outArray = msgPack2String.split("").map((char) => char.charCodeAt(0));

  const msgPack2 = Uint8Array.from(outArray);

  const json = decode(msgPack2);

  console.log("Test Decode Config", json);

  ///

  return b64String;
}
