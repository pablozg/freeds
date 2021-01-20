void sendUDPRequest(void)
{
    //   inverterUDP.beginPacket("79.99.215.248", 8899);
      inverterUDP.beginPacket(config.sensor_ip, 8899);
      inverterUDP.write(0xAA);
      inverterUDP.write(0x55);
      inverterUDP.write(0xC0);
      inverterUDP.write(0x7F);
      inverterUDP.write(0x01);
      inverterUDP.write(0x06);
      inverterUDP.write(0x00);
      inverterUDP.write(0x02);
      inverterUDP.write(0x45);
      inverterUDP.endPacket();
    //   INFOV("MENSAJE UDP enviado\n");
}

void parseUDP(void)
{
  int16_t value = 0;
  uint16_t uvalue = 0;
  
  int packetSize = inverterUDP.parsePacket();
  
  // receive incoming UDP packets
  if (packetSize)
  {
    INFOV("Received %d bytes from %s, port %d\n", packetSize, inverterUDP.remoteIP().toString().c_str(), inverterUDP.remotePort());
    
    int len = inverterUDP.read(incomingPacket, 1024);
    
    if (len > 0) { incomingPacket[len] = 0; }

    // for (int i = 0; i < packetSize; i++) {
    //   INFOV("HEX pos %d: %02X\n", i, incomingPacket[i]);
    // }
 
    // Pos 7 de diferencia con python

    // PV1 Volts
    uvalue = (incomingPacket[7] << 8) | (incomingPacket[8]);
    inverter.pv1v = (float)uvalue / 10.0;

    // PV1 Amps
    uvalue = (incomingPacket[9] << 8) | (incomingPacket[10]);
    inverter.pv1c = (float)uvalue / 10.0;

    // PV1 Power
    inverter.pw1 = inverter.pv1v * inverter.pv1c;

    // PV2 Volts
    uvalue = (incomingPacket[12] << 8) | (incomingPacket[13]);
    inverter.pv2v = (float)uvalue / 10.0;

    // PV2 Amps
    uvalue = (incomingPacket[14] << 8) | (incomingPacket[15]);
    inverter.pv2c = (float)uvalue / 10.0;

    // PV2 Power
    inverter.pw2 = inverter.pv2v * inverter.pv2c;

    // PV Total Power
    inverter.wsolar = inverter.pw1 + inverter.pw2;

    // Battery Volts
    uvalue = (incomingPacket[17] << 8) | (incomingPacket[18]);
    meter.voltage = (float)uvalue / 10.0;

    // Battery Amps
    value = (incomingPacket[25] << 8) | (incomingPacket[26]);
    meter.current = (float)value / 10.0;

    // battery Power
    inverter.batteryWatts = meter.voltage * meter.current;

    // Battery SoC
    uvalue = (incomingPacket[33] << 8) | (incomingPacket[34]);
    inverter.batterySoC = (float)uvalue;

    // Grid Voltage
    uvalue = (incomingPacket[41] << 8) | (incomingPacket[42]);
    inverter.gridv = (float)uvalue / 10.0;

    // Grid Current
    // uvalue = (incomingPacket[43] << 8) | (incomingPacket[44]);
    // meter.current = (float)uvalue / 10.0;
    
    // Grid Power
    value = (incomingPacket[45] << 8) | (incomingPacket[46]);
    inverter.wgrid = value;

    uvalue = (incomingPacket[87] << 8) | (incomingPacket[88]);

    uvalue == 2 ? inverter.wgrid *= -1 : inverter.wgrid *= 1;

    // House comsumption
    inverter.loadWatts = inverter.wsolar + inverter.batteryWatts - inverter.wgrid;

    config.flags.changeGridSign ? inverter.wgrid *= -1 : inverter.wgrid *= 1;
    
    // Registro 52
    uvalue = (incomingPacket[47] << 8) | (incomingPacket[48]);
    meter.frequency = (float)uvalue / 100.0;

    uvalue = (incomingPacket[76] << 8) | (incomingPacket[77]);
    inverter.wtoday = uvalue / 10.0;

    // Registro 58
    uvalue = (incomingPacket[60] << 8) | (incomingPacket[61]);
    inverter.temperature = uvalue / 10.0;

    Error.RecepcionDatos = false;
    timers.ErrorRecepcionDatos = millis();
  }
}