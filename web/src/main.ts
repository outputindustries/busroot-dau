import modbusDevices from "./modbus-devices";
import { generateDauConfig, generateDauToken } from "./utils";

declare global {
  interface Window {
    dfu: any;
    dfuse: any;
  }
}

window.dfu = window.dfu || {};
window.dfuse = window.dfuse || {};

let dfuseDevice;
let firmwareM7;
let firmwareM4;
let statusText;
let configMethodInput: HTMLSelectElement;
let deviceIdInput;
let wifiSsidInput;
let wifiPasswordInput;
let communicationModeInput;
let mqttServerInput;
let mqttPortInput;
let mqttUsernameInput;
let mqttPasswordInput;
let mqttClientIdInput;
let configFieldsDiv;
let wifiSettingsDiv;
let mqttSettingsDiv;
let modbusSettingsDiv;
let modbusDeviceNameInput: HTMLSelectElement;
let modbusDeviceCountInput: HTMLSelectElement;

function updateStatusText(newStatus) {
  console.log(newStatus);

  if (statusText) {
    statusText.innerText = newStatus;
  }
}

async function main() {
  const appDiv = document.querySelector("#app") as HTMLElement;
  const connectButton = document.querySelector("#connect") as HTMLButtonElement;
  const disconnectButton = document.querySelector(
    "#disconnect"
  ) as HTMLButtonElement;
  const uploadButton = document.querySelector(
    "#upload"
  ) as HTMLButtonElement;
  statusText = document.querySelector("#status") as HTMLElement;

  configMethodInput = document.querySelector("#config-method") as HTMLSelectElement;
  deviceIdInput = document.querySelector("#device-id");
  wifiSsidInput = document.querySelector("#wifi-ssid");
  wifiPasswordInput = document.querySelector("#wifi-password");
  communicationModeInput = document.querySelector("#communication-mode");
  mqttServerInput = document.querySelector("#mqtt-server");
  mqttPortInput = document.querySelector("#mqtt-port");
  mqttUsernameInput = document.querySelector("#mqtt-username");
  mqttPasswordInput = document.querySelector("#mqtt-password");
  mqttClientIdInput = document.querySelector("#mqtt-client-id");
  configFieldsDiv = document.querySelector("#config-fields");
  wifiSettingsDiv = document.querySelector("#wifi-settings");
  mqttSettingsDiv = document.querySelector("#mqtt-settings");
  modbusSettingsDiv = document.querySelector("#modbus-settings");
  modbusDeviceNameInput = document.querySelector(
    "#modbus-device-name"
  ) as HTMLSelectElement;
  modbusDeviceCountInput = document.querySelector(
    "#modbus-device-count"
  ) as HTMLSelectElement;

  var urlParams = new URLSearchParams(window.location.search);

  if (urlParams.has("device-id")) {
    deviceIdInput.value = urlParams.get("device-id");
  }
  if (urlParams.has("communication-mode")) {
    communicationModeInput.value = urlParams.get("communication-mode");
  }
  if (urlParams.has("wifi-ssid")) {
    wifiSsidInput.value = urlParams.get("wifi-ssid");
  }
  if (urlParams.has("wifi-password")) {
    wifiPasswordInput.value = urlParams.get("wifi-password");
  }
  if (urlParams.has("mqtt-server")) {
    mqttServerInput.value = urlParams.get("mqtt-server");
  }
  if (urlParams.has("mqtt-port")) {
    mqttPortInput.value = urlParams.get("mqtt-port");
  }
  if (urlParams.has("mqtt-username")) {
    mqttUsernameInput.value = urlParams.get("mqtt-username");
  }
  if (urlParams.has("mqtt-password")) {
    mqttPasswordInput.value = urlParams.get("mqtt-password");
  }
  if (urlParams.has("mqtt-client-id")) {
    mqttClientIdInput.value = urlParams.get("mqtt-client-id");
  }
  if (urlParams.has("modbus-device-count")) {
    modbusDeviceCountInput.value = urlParams.get("modbus-device-count");
  }

  for (const device of modbusDevices) {
    const option = document.createElement("option");
    option.text = device.name;
    modbusDeviceNameInput.options.add(option);
  }

  if (urlParams.has("modbus-device-name")) {
    modbusDeviceNameInput.value = urlParams.get("modbus-device-name");
  }

  if (!navigator.usb) {
    appDiv.style.display = "none";
    updateStatusText("Browser not supported. Use on Chrome.");
    return;
  }

  connectButton.disabled = false;
  disconnectButton.disabled = true;

  try {
    const blobM7 = await fetch(
      "./firmware/busroot-dau-opta_m7.bin",
      { cache: "no-store" }
    ).then((r) => {
      if (!r.ok)
        throw new Error(
          `Failed to fetch M7 firmware: ${r.status} ${r.statusText}`
        );
      return r.blob();
    });
    const blobM4 = await fetch(
      "./firmware/busroot-dau-opta_m4.bin",
      { cache: "no-store" }
    ).then((r) => {
      if (!r.ok)
        throw new Error(
          `Failed to fetch M4 firmware: ${r.status} ${r.statusText}`
        );
      return r.blob();
    });

    let readerM7 = new FileReader();
    readerM7.onload = function () {
      firmwareM7 = readerM7.result;
    };
    readerM7.onerror = function () {
      updateStatusText("Failed to read M7 firmware file");
      console.error("FileReader error:", readerM7.error);
    };
    readerM7.readAsArrayBuffer(blobM7);

    let readerM4 = new FileReader();
    readerM4.onload = function () {
      firmwareM4 = readerM4.result;
    };
    readerM4.onerror = function () {
      updateStatusText("Failed to read M4 firmware file");
      console.error("FileReader error:", readerM4.error);
    };
    readerM4.readAsArrayBuffer(blobM4);
  } catch (error) {
    console.error("Firmware fetch error:", error);
    updateStatusText(
      "Failed to upload firmware files. Please check your connection and try again."
    );
    connectButton.disabled = true;
  }

  connectButton.addEventListener("click", function () {
    navigator.usb
      .requestDevice({ filters: [] })
      .then(async (selectedDevice) => {
        connectButton.disabled = true;
        disconnectButton.disabled = false;

        connectDfu(selectedDevice);
      })
      .catch((error) => {
        console.log(error);
      });
  });
  disconnectButton.addEventListener("click", async function () {
    const devices = await navigator.usb.getDevices();

    for (const device of devices) {
      device.close();
    }

    disconnectButton.disabled = true;
    connectButton.disabled = false;
    updateStatusText(" ");
  });

  uploadButton.addEventListener("click", function () {
    uploadFirmware();
  });

  // Add event listener for configuration method changes
  configMethodInput.addEventListener(
    "change",
    updateConfigFieldsVisibility
  );
  // Set initial visibility
  updateConfigFieldsVisibility();

  // Add event listener for communication mode changes
  communicationModeInput.addEventListener(
    "change",
    updateNetworkSettingsVisibility
  );
  // Set initial visibility
  updateNetworkSettingsVisibility();

  // Add event listener for Modbus device changes
  modbusDeviceNameInput.addEventListener(
    "change",
    updateModbusDeviceCountVisibility
  );
  // Set initial visibility
  updateModbusDeviceCountVisibility();
}

async function connectDfu(usbDevice) {
  let interfaces = window.dfu.findDeviceDfuInterfaces(usbDevice);

  await fixInterfaceNames(usbDevice, interfaces);

  const dfuDevice = new window.dfu.Device(usbDevice, interfaces[0]);

  await dfuDevice.open();

  const desc = await getDFUDescriptorProperties(dfuDevice);

  dfuDevice.properties = desc;

  dfuseDevice = new window.dfuse.Device(dfuDevice.device_, dfuDevice.settings);

  updateStatusText("Connected to '" + dfuseDevice.device_.productName + "'.");
}

async function uploadFirmware() {
  let firmwareConfig: Uint8Array;

  try {
    const modbusDevice = modbusDevices.find(
      (device) => device.name === modbusDeviceNameInput.value
    );

    const dauConfig = generateDauConfig({
      deviceId: deviceIdInput.value,
      communicationMode: communicationModeInput.value,
      wifiSsid: wifiSsidInput.value,
      wifiPassword: wifiPasswordInput.value,
      mqttServer: mqttServerInput.value,
      mqttPort: parseInt(mqttPortInput.value),
      mqttUsername: mqttUsernameInput.value,
      mqttPassword: mqttPasswordInput.value,
      mqttClientId: mqttClientIdInput.value,
      modbusDeviceAddresses: modbusDevice?.addresses,
      modbusDeviceCount: parseInt(modbusDeviceCountInput.value),
      modbusRegisterStyle: modbusDevice?.registerStyle
    });

    console.log("DAU Config", dauConfig);

    const dauToken = generateDauToken(dauConfig);

    console.log("DAU Token", dauToken);

    var enc = new TextEncoder();
    firmwareConfig = enc.encode(dauToken);
  } catch (error) {
    updateStatusText("Invalid Config");
    console.log("Token Generate Error", error);
    return;
  }

  updateStatusText("Uploading M7 firmware to device...");

  dfuseDevice.startAddress = parseInt("0x08040000", 16);
  await dfuseDevice.do_download(4096, firmwareM7, false, false);

  updateStatusText("Uploading M4 firmware to device...");
  dfuseDevice.startAddress = parseInt("0x08100000", 16);
  await dfuseDevice.do_download(4096, firmwareM4, false, false);

  // Only upload config if "Configure now and upload with firmware" is selected
  if (configMethodInput.value === "upload") {
    updateStatusText("Uploading config to device...");
    dfuseDevice.startAddress = parseInt("0x080C0000", 16);
    await dfuseDevice.do_download(4096, firmwareConfig, false, true);
  } else {
    updateStatusText("Skipping config upload (will configure via serial interface)...");
    // Still need to trigger the final manifest phase
    dfuseDevice.startAddress = parseInt("0x080C0000", 16);
    await dfuseDevice.do_download(4096, new Uint8Array(0), false, true);
  }

  updateStatusText("Firmware upload complete.");
}

document.addEventListener("DOMContentLoaded", () => {
  main();
});

async function fixInterfaceNames(device_, interfaces) {
  // Check if any interface names were not read correctly
  if (interfaces.some((intf) => intf.name == null)) {
    // Manually retrieve the interface name string descriptors
    let tempDevice = new window.dfu.Device(device_, interfaces[0]);
    await tempDevice.device_.open();
    await tempDevice.device_.selectConfiguration(1);
    let mapping = await tempDevice.readInterfaceNames();
    await tempDevice.close();

    for (let intf of interfaces) {
      if (intf.name === null) {
        let configIndex = intf.configuration.configurationValue;
        let intfNumber = intf["interface"].interfaceNumber;
        let alt = intf.alternate.alternateSetting;
        intf.name = mapping[configIndex][intfNumber][alt];
      }
    }
  }
}

function getDFUDescriptorProperties(device) {
  // Attempt to read the DFU functional descriptor
  // TODO: read the selected configuration's descriptor
  return device.readConfigurationDescriptor(0).then(
    (data) => {
      let configDesc = window.dfu.parseConfigurationDescriptor(data);
      let funcDesc: any = null;
      let configValue = device.settings.configuration.configurationValue;
      if (configDesc.bConfigurationValue == configValue) {
        for (let desc of configDesc.descriptors) {
          if (
            desc.bDescriptorType == 0x21 &&
            desc.hasOwnProperty("bcdDFUVersion")
          ) {
            funcDesc = desc;
            break;
          }
        }
      }

      if (funcDesc) {
        return {
          WillDetach: (funcDesc.bmAttributes & 0x08) != 0,
          ManifestationTolerant: (funcDesc.bmAttributes & 0x04) != 0,
          CanUpload: (funcDesc.bmAttributes & 0x02) != 0,
          CanDnload: (funcDesc.bmAttributes & 0x01) != 0,
          TransferSize: funcDesc.wTransferSize,
          DetachTimeOut: funcDesc.wDetachTimeOut,
          DFUVersion: funcDesc.bcdDFUVersion,
        };
      } else {
        return {};
      }
    },
    (error) => {
      console.log(error);
    }
  );
}

function updateConfigFieldsVisibility() {
  if (configFieldsDiv && configMethodInput) {
    configFieldsDiv.style.display =
      configMethodInput.value === "upload" ? "block" : "none";
  }
}

function updateNetworkSettingsVisibility() {
  if (wifiSettingsDiv && communicationModeInput) {
    wifiSettingsDiv.style.display =
      communicationModeInput.value === "WIFI" ? "block" : "none";
  }
  if (mqttSettingsDiv && communicationModeInput) {
    mqttSettingsDiv.style.display =
      communicationModeInput.value === "NONE" ? "none" : "block";
  }
}

function updateModbusDeviceCountVisibility() {
  if (modbusSettingsDiv && modbusDeviceNameInput) {
    modbusSettingsDiv.style.display =
      modbusDeviceNameInput.value !== "None" ? "block" : "none";
  }
}
