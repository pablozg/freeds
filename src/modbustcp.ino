/*
  modbustcp.ino - FreeDs modbus tcp rutines
  Derivador de excedentes para ESP32 DEV Kit // Wifi Kit 32
  
  Copyright (C) 2020 Pablo Zerón (https://github.com/pablozg/freeds)

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

boolean data_ready = false;
boolean froniusRequestSend = false;
uint8_t froniusRegisterNum = 0;

enum valueType
{
    ENUM,     // enumeration
    U16FIX0,  // 16 bits unsigned, no decimals
    U16FIX1,  // 16 bits unsigned, 1 decimals (/10)
    U16FIX2,  // 16 bits unsigned, 2 decimals (/100)
    U16FIX3,  // 16 bits unsigned, 3 decimals (/1000)
    U16FIX10, // 16 bits unsigned, plus 10 (*10)
    U32FIX0,  // 32 bits unsigned, no decimals
    U32FIX1,  // 32 bits unsigned, 1 decimals
    U32FIX2,  // 32 bits unsigned, 2 decimals
    U32FIX3,  // 32 bits unsigned, 3 decimals
    U64FIX0,  // 64 bits unsigned, no decimals
    U64FIX1,  // 64 bits unsigned, 1 decimals
    U64FIX2,  // 64 bits unsigned, 2 decimals
    U64FIX3,  // 64 bits unsigned, 3 decimals
    S16FIX0,  // 16 bits signed, no decimals
    S16FIX1,  // 16 bits signed, 1 decimals (/10)
    S16FIX2,  // 16 bits signed, 2 decimals (/100)
    S16FIX3,  // 16 bits signed, 3 decimals (/1000)
    S16FIX10, // 16 bits signed, plus 10 (*10)
    S32FIX0,  // 32 bits signed, no decimals
    S32FIX1,  // 32 bits signed, 1 decimals
    S32FIX2,  // 32 bits signed, 2 decimals
    S32FIX3,  // 32 bits signed, 3 decimals
    F32FIX0,  // 32 bits float, 0 decimals
    F32FIX1,  // 32 bits float, 1 decimals
    F32FIX2,  // 32 bits float, 2 decimals
    F32FIX3,  // 32 bits float, 3 decimals
    FRONIUSSOLTOD,
    FRONIUSPV1,
    FRONIUSPV2,
};

struct registerData
{
    float *variable;
    uint8_t serverID;
    uint16_t address;
    uint16_t length;
    valueType type;
};

registerData smaRegisters[] = {
    &inverter.wtoday, 3, 30517, 2, U32FIX0,
    &inverter.pv1c, 3, 30769, 2, S32FIX3,
    &inverter.pv1v, 3, 30771, 2, S32FIX2,
    &inverter.pw1, 3, 30773, 2, S32FIX0,
    &inverter.wsolar, 3, 30775, 2, S32FIX0,
    &inverter.gridv, 3, 30783, 2, U32FIX2,
    &inverter.pv2c, 3, 30957, 2, S32FIX3,
    &inverter.pv2v, 3, 30959, 2, S32FIX2,
    &inverter.pw2, 3, 30961, 2, S32FIX0,
    &inverter.wgrid, 3, 31259, 2, U32FIX0
};

registerData victronRegisters[] = {
    &inverter.wgrid, 100, 820, 1, S16FIX0,
    &meter.voltage, 100, 840, 1, U16FIX1,
    &meter.current, 100, 841, 1, S16FIX1,
    &inverter.batteryWatts, 100, 842, 1, S16FIX0,
    &inverter.batterySoC, 100, 843, 1, U16FIX0,
    &inverter.wsolar, 100, 850, 1, U16FIX0
};

// registerData froniusRegisters[] = {
//     &inverter.wsolar, 1, 499, 2, S32FIX0,
//     &inverter.wtoday, 1, 501, 4, U64FIX3,
//     &inverter.wgrid, 1, 40091, 2, F32FIX1,
//     &inverter.pv1c, 1, 40282, 1, U16FIX2,
//     &inverter.pv1v, 1, 40283, 1, U16FIX2,
//     &inverter.pw1, 1, 40284, 1, U16FIX2,
//     &inverter.pv2c, 1, 40302, 1, U16FIX2,
//     &inverter.pv2v, 1, 40303, 1, U16FIX2,
//     &inverter.pw2, 1, 40304, 1, U16FIX2
//     // &inverter.gridv, 240, 40079, 2, U32FIX1
// };

registerData froniusRegisters[] = {
    &inverter.wsolar, 1, 499, 6, FRONIUSSOLTOD,
    &inverter.wgrid, 1, 40091, 2, F32FIX1,
    &inverter.pv1c, 1, 40282, 3, FRONIUSPV1,
    &inverter.pv2c, 1, 40302, 3, FRONIUSPV2
    // &inverter.gridv, 240, 40079, 2, U32FIX1
};

registerData huaweiRegisters[] = {
    &inverter.pv1v, 0, 32016, 1, S16FIX1,
    &inverter.pv1c, 0, 32017, 1, S16FIX2,
    &inverter.pv2v, 0, 32018, 1, S16FIX1,
    &inverter.pv2c, 0, 32019, 1, S16FIX2,
    &inverter.wsolar, 0, 32064, 2, S32FIX0,
    &inverter.wtoday, 0, 32114, 2, U32FIX2,
    &inverter.wgrid, 0, 37113, 2, S32FIX1
};

void parseFroniusSolarToday(uint8_t *data)
{
  inverter.wsolar = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | (data[3]);
  uint32_t high = 0;
  uint32_t low = 0;
  uint64_t value = 0;
  high = (data[4] << 24) | (data[5] << 16) | (data[6] << 8) | (data[7]);
  low = (data[8] << 24) | (data[9] << 16) | (data[10] << 8) | (data[11]);
  value = (((uint64_t) high) << 32) | ((uint64_t) low);
  inverter.wtoday = (float)value / 1000.0;
}

void parseFroniusPV1(uint8_t *data)
{
  uint16_t value = 0;
  value = (data[0] << 8) | (data[1]);
  inverter.pv1c = (float)value / 100.0;
  value = (data[2] << 8) | (data[3]);
  inverter.pv1v = (float)value / 100.0;
  value = (data[4] << 8) | (data[5]);
  inverter.pw1 = (float)value / 100.0;
}

void parseFroniusPV2(uint8_t *data)
{
  uint16_t value = 0;
  value = (data[0] << 8) | (data[1]);
  inverter.pv2c = (float)value / 100.0;
  value = (data[2] << 8) | (data[3]);
  inverter.pv2v = (float)value / 100.0;
  value = (data[4] << 8) | (data[5]);
  inverter.pw2 = (float)value / 100.0;
}

float parseFloat32(uint8_t *data, int precision)
{
    float value = 0;
    *((unsigned char *)&value + 3) = data[0];
    *((unsigned char *)&value + 2) = data[1];
    *((unsigned char *)&value + 1) = data[2];
    *((unsigned char *)&value + 0) = data[3];
    Serial.printf("Data F32: 0x%.2X 0x%.2X 0x%.2X 0x%.2X\n", data[0], data[1], data[2], data[3]);
    Serial.printf("Float 32: %.6f\n", value);

    switch (precision)
    {
        case 0: { return value; break; }
        case 1: { return (value / 10.0); break; }
        case 2: { return (value / 100.0); break; }
        case 3: { return (value / 1000.0); break; }
    }
    return 0;
}

float parseUnsigned16(uint8_t *data, int precision)
{
    uint16_t value = 0;
    value = (data[0] << 8) | (data[1]);
    Serial.printf("unsigned 16: %" PRIu16 "\n",value);

    switch (precision)
    {
        case 0: { return (float)value; break; }
        case 1: { return ((float)value / 10.0); break; }
        case 2: { return ((float)value / 100.0); break; }
        case 3: { return ((float)value / 1000.0); break; }
        case 10: { return ((float)value * 10.0); break; }
    }
    return 0;
}

float parseUnsigned32(uint8_t *data, int precision)
{
    uint32_t value = 0;
    value = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | (data[3]);
    Serial.printf("Data U32: 0x%.2X 0x%.2X 0x%.2X 0x%.2X\n", data[0], data[1], data[2], data[3]);
    Serial.printf("Unsigned 32: %" PRIu32 "\n",value);

    switch (precision)
    {
        case 0: { return (float)value; break; }
        case 1: { return ((float)value / 10.0); break; }
        case 2: { return ((float)value / 100.0); break; }
        case 3: { return ((float)value / 1000.0); break; }
    }
    return 0;
}

float parseUnsigned64(uint8_t *data, int precision)
{
    uint32_t high = 0;
    uint32_t low = 0;
    uint64_t value = 0;
    high = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | (data[3]);
    low = (data[4] << 24) | (data[5] << 16) | (data[6] << 8) | (data[7]);
    value = (((uint64_t) high) << 32) | ((uint64_t) low);
    Serial.printf("Data U64 High: 0x%.2X 0x%.2X 0x%.2X 0x%.2X\n", data[0], data[1], data[2], data[3]);
    Serial.printf("Data U64 low: 0x%.2X 0x%.2X 0x%.2X 0x%.2X\n", data[4], data[5], data[6], data[7]);
    Serial.printf("Unsigned 64: %" PRIu64 "\n",value);

    switch (precision)
    {
        case 0: { return (float)value; break; }
        case 1: { return ((float)value / 10.0); break; }
        case 2: { return ((float)value / 100.0); break; }
        case 3: { return ((float)value / 1000.0); break; }
    }
    return 0;
}

float parseSigned16(uint8_t *data, int precision)
{
    int16_t value = 0;
    value = (data[0] << 8) | (data[1]);
    Serial.printf("signed 16: %" PRId16 "\n",value);

    switch (precision)
    {
        case 0: { return (float)value; break; }
        case 1: { return ((float)value / 10.0); break; }
        case 2: { return ((float)value / 100.0); break; }
        case 3: { return ((float)value / 1000.0); break; }
        case 10: { return ((float)value * 10.0); break; }
    }
    return 0;
}

float parseSigned32(uint8_t *data, int precision)
{
    int32_t value = 0;
    value = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | (data[3]);
    Serial.printf("Data S32: 0x%.2X 0x%.2X 0x%.2X 0x%.2X\n", data[0], data[1], data[2], data[3]);
    Serial.printf("Signed 32: %" PRId32 "\n",value);

    switch (precision)
    {
        case 0: { return (float)value; break; }
        case 1: { return ((float)value / 10.0); break; }
        case 2: { return ((float)value / 100.0); break; }
        case 3: { return ((float)value / 1000.0); break; }
    }
    return 0;
}

void configModbusTcp(void)
{
    modbustcp->onData([](uint16_t packet, uint8_t slave, esp32Modbus::FunctionCode fc , uint8_t* data , uint16_t len, void* arg) {
        registerData* a = reinterpret_cast<registerData*>(arg);
        switch (a->type)
        {
            case ENUM:
            case U16FIX0:  { *a->variable = parseUnsigned16(data, 0); break; }
            case U16FIX1:  { *a->variable = parseUnsigned16(data, 1); break; }
            case U16FIX2:  { *a->variable = parseUnsigned16(data, 2); break; }
            case U16FIX3:  { *a->variable = parseUnsigned16(data, 3); break; }
            case U16FIX10: { *a->variable = parseUnsigned16(data, 10); break; }
            case U32FIX0:  { *a->variable = parseUnsigned32(data, 0); break; }
            case U32FIX1:  { *a->variable = parseUnsigned32(data, 1); break; }
            case U32FIX2:  { *a->variable = parseUnsigned32(data, 2); break; }
            case U32FIX3:  { *a->variable = parseUnsigned32(data, 3); break; }
            case U64FIX0:  { *a->variable = parseUnsigned64(data, 0); break; }
            case U64FIX1:  { *a->variable = parseUnsigned64(data, 1); break; }
            case U64FIX2:  { *a->variable = parseUnsigned64(data, 2); break; }
            case U64FIX3:  { *a->variable = parseUnsigned64(data, 3); break; }
            case S16FIX0:  { *a->variable = parseSigned16(data, 0); break; }
            case S16FIX1:  { *a->variable = parseSigned16(data, 1); break; }
            case S16FIX2:  { *a->variable = parseSigned16(data, 2); break; }
            case S16FIX3:  { *a->variable = parseSigned16(data, 3); break; }
            case S16FIX10: { *a->variable = parseSigned16(data, 10); break; }
            case S32FIX0:  { *a->variable = parseSigned32(data, 0); break; }
            case S32FIX1:  { *a->variable = parseSigned32(data, 1); break; }
            case S32FIX2:  { *a->variable = parseSigned32(data, 2); break; }
            case S32FIX3:  { *a->variable = parseSigned32(data, 3); break; }
            case F32FIX0:  { *a->variable = parseFloat32(data, 0); break; }
            case F32FIX1:  { *a->variable = parseFloat32(data, 1); break; }
            case F32FIX2:  { *a->variable = parseFloat32(data, 2); break; }
            case F32FIX3:  { *a->variable = parseFloat32(data, 3); break; }
            case FRONIUSSOLTOD: { parseFroniusSolarToday(data); break; }
            case FRONIUSPV1: { parseFroniusPV1(data); break; }
            case FRONIUSPV2: { parseFroniusPV2(data); break; }
        }
        if (a->address == 820 && config.wversion == 14) { inverter.wgrid *= -1.0; }
        if (config.wversion == 15) { data_ready = true; froniusRegisterNum++; }
        Error.RecepcionDatos = false;
        timers.ErrorRecepcionDatos = millis();
       return;
    });
    modbustcp->onError([](uint16_t packet, esp32Modbus::Error e, void* arg) {
      registerData* a = reinterpret_cast<registerData*>(arg);
      Serial.printf("Error packet in address %d (%u): %02x\n", a->address, packet, e);
      froniusRequestSend = false;
    });
}

void sma(void)
{
    if (modbustcp == NULL) {
        modbustcp = new esp32ModbusTCP(modbusIP, 502);
        configModbusTcp();
    }

    for (uint8_t i = 0; i < sizeOfArray(smaRegisters); ++i) {

        if (modbustcp->readHoldingRegisters(smaRegisters[i].serverID, smaRegisters[i].address, smaRegisters[i].length, &(smaRegisters[i])) > 0) {
            //Serial.printf("  requested %d\n", smaRegisters[i].address);
            // Error.RecepcionDatos = false;
            // timers.ErrorRecepcionDatos = millis();
        } else {
            Serial.printf("  error requesting address %d\n", smaRegisters[i].address);
        }
    }
}

void victron(void)
{
    if (modbustcp == NULL) {
        modbustcp = new esp32ModbusTCP(modbusIP, 502);
        configModbusTcp();
    }

    for (uint8_t i = 0; i < sizeOfArray(victronRegisters); ++i) {
        if (modbustcp->readHoldingRegisters(victronRegisters[i].serverID, victronRegisters[i].address, victronRegisters[i].length, &(victronRegisters[i])) > 0) {
            //Serial.printf("  requested %d\n", victronRegisters[i].address);
            // Error.RecepcionDatos = false;
            // timers.ErrorRecepcionDatos = millis();
        } else {
            Serial.printf("  error requesting address %d\n", victronRegisters[i].address);
        }
    }
}

// void fronius(void)
// {
//     if (modbustcp == NULL) {
//         modbustcp = new esp32ModbusTCP(modbusIP, 502);
//         configModbusTcp();
//     }

//     for (uint8_t i = 0; i < sizeOfArray(froniusRegisters); ++i) {
//         if (modbustcp->readHoldingRegisters(froniusRegisters[i].serverID, froniusRegisters[i].address, froniusRegisters[i].length, &(froniusRegisters[i])) > 0) {
//             //Serial.printf("  requested %d\n", froniusRegisters[i].address);
//             Error.RecepcionDatos = false;
//             timers.ErrorRecepcionDatos = millis();
//         } else {
//             Serial.printf("  error requesting address %d\n", froniusRegisters[i].address);
//         }
//     }
// }

void fronius(void)
{
    if (modbustcp == NULL) {
        modbustcp = new esp32ModbusTCP(modbusIP, 502);
        configModbusTcp();
    }

    if (froniusRegisterNum >= sizeOfArray(froniusRegisters)) { froniusRegisterNum = 0; }

    if (data_ready || !froniusRequestSend) {
      data_ready = false;
      froniusRequestSend = false;
      if (modbustcp->readHoldingRegisters(froniusRegisters[froniusRegisterNum].serverID, froniusRegisters[froniusRegisterNum].address, froniusRegisters[froniusRegisterNum].length, &(froniusRegisters[froniusRegisterNum])) > 0) {
          Serial.printf("  requested %d\n", froniusRegisters[froniusRegisterNum].address);
          // Error.RecepcionDatos = false;
          // timers.ErrorRecepcionDatos = millis();
          froniusRequestSend = true;
      } else {
          Serial.printf("  error requesting address %d\n", froniusRegisters[froniusRegisterNum].address);
          froniusRequestSend = false;
      }
    }
}

void huawei(void)
{
    if (modbustcp == NULL) {
        modbustcp = new esp32ModbusTCP(modbusIP, 502);
        configModbusTcp();
    }

    for (uint8_t i = 0; i < sizeOfArray(huaweiRegisters); ++i) {
        if (modbustcp->readHoldingRegisters(huaweiRegisters[i].serverID, huaweiRegisters[i].address, huaweiRegisters[i].length, &(huaweiRegisters[i])) > 0) {
            //Serial.printf("  requested %d\n", huaweiRegisters[i].address);
            // Error.RecepcionDatos = false;
            // timers.ErrorRecepcionDatos = millis();
        } else {
            Serial.printf("  error requesting address %d\n", huaweiRegisters[i].address);
        }
    }
    inverter.pw1 = inverter.pv1v * inverter.pv1c;
    inverter.pw2 = inverter.pv2v * inverter.pv2c;
}