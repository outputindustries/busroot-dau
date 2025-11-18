export interface ModbusDeviceAddresses {
  p1Volts: number;
  p2Volts: number;
  p3Volts: number;
  p1Amps: number;
  p2Amps: number;
  p3Amps: number;
  pf: number;
  kWh: number;
}

export interface ModbusDevice {
  name: string;
  addresses: ModbusDeviceAddresses;
  registerStyle: 0 | 1;
}

const devices: ModbusDevice[] = [
  {
    name: "RS PRO - 236-929X",
    addresses: {
      p1Volts: 0x00,
      p2Volts: 0x02,
      p3Volts: 0x04,
      p1Amps: 0x06,
      p2Amps: 0x08,
      p3Amps: 0x0a,
      pf: 0x3e,
      kWh: 0x0156
    },
    registerStyle: 0
  },
  {
    name: "Carlo Gavazzi - EM210",
    addresses: {
      p1Volts: 0x00,
      p2Volts: 0x02,
      p3Volts: 0x04,
      p1Amps: 0x0c,
      p2Amps: 0x0e,
      p3Amps: 0x10,
      pf: 0x31,
      kWh: 0x34
    },
    registerStyle: 1
  },
];

export default devices;
