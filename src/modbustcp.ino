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

struct FRONIUS
{
    boolean froniusRequestSend = false;
    uint32_t sendTimeOut;
    uint8_t froniusRegisterNum = 0;
    uint16_t scaleFactorA; // Current
    uint16_t scaleFactorV; // voltage
    uint16_t scaleFactorW; // Power
} froniusVariables;

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
    FRONIUSSCALE,
    SUNNYBOYGRID,
    SOLAREDGEINVERTER,
    SOLAREDGEMETER,
    WIBEEEMODBUS,
    SCHNEIDERMODBUS1,
    SCHNEIDERMODBUS2,
    INGETEAMMODBUS
};

struct registerData
{
    float *variable;
    uint8_t serverID;
    uint16_t address;
    uint16_t length;
    valueType type;
};

registerData smaRegistersBoy[] = { // Sunny Boy
    &inverter.wtoday, 3, 30535, 2, U32FIX3,
    &inverter.pv1c, 3, 30769, 2, S32FIX3,
    &inverter.pv1v, 3, 30771, 2, S32FIX2,
    &inverter.pw1, 3, 30773, 2, S32FIX0,
    &inverter.wsolar, 3, 30775, 2, S32FIX0,
    &inverter.pv2c, 3, 30957, 2, S32FIX3,
    &inverter.pv2v, 3, 30959, 2, S32FIX2,
    &inverter.pw2, 3, 30961, 2, S32FIX0,
    &inverter.wgrid, 3, 30865, 2, S32FIX0,
    &inverter.wgrid, 3, 30867, 2, SUNNYBOYGRID
};

registerData smaRegistersIsland[] = { // Sunny Island
    &inverter.batterySoC, 3, 30845, 2, U32FIX0,
    &inverter.batteryWatts, 3, 30775, 2, S32FIX0,
    &inverter.temperature, 3, 30849, 2, S32FIX1,
    &meter.current, 3, 30843, 2, S32FIX3,
    &meter.voltage, 3, 30851, 2, U32FIX2
};

registerData victronRegisters[] = {
    &meter.voltage, 100, 840, 1, U16FIX1,
    &meter.current, 100, 841, 1, S16FIX1,
    &inverter.batteryWatts, 100, 842, 1, S16FIX0,
    &inverter.batterySoC, 100, 843, 1, U16FIX0,
    &inverter.acIn, 100, 811, 1, U16FIX0,
    &inverter.acOut, 100, 808, 1, U16FIX0,
    &inverter.wsolar, 100, 850, 1, U16FIX0,
    &inverter.wgrid, 100, 820, 1, S16FIX0,
    &inverter.loadWatts, 100, 817, 1, U16FIX0
};

registerData froniusRegisters[] = {
    &inverter.wsolar, 1, 499, 6, FRONIUSSOLTOD,
    &inverter.pv1c, 1, 40265, 3, FRONIUSSCALE,
    &inverter.pv1c, 1, 40282, 3, FRONIUSPV1,
    &inverter.pv2c, 1, 40302, 3, FRONIUSPV2,
    &inverter.wgrid, 240, 40097, 2, F32FIX0,
    &meter.voltage, 240, 40079, 2, F32FIX0,
    &meter.current, 240, 40071, 2, F32FIX0
};

registerData huaweiRegisters[] = {
    &inverter.wgrid, 0, 37113, 2, S32FIX0,
    &inverter.wtoday, 0, 32114, 2, U32FIX2,
    &inverter.wsolar, 0, 32064, 2, S32FIX0,
    &inverter.pv1v, 0, 32016, 1, S16FIX1,
    &inverter.pv1c, 0, 32017, 1, S16FIX2,
    &inverter.pv2v, 0, 32018, 1, S16FIX1,
    &inverter.pv2c, 0, 32019, 1, S16FIX2,
    &inverter.temperature, 0, 32087, 1, S16FIX1,
    &inverter.batteryWatts, 0, 37001, 2, S32FIX0
};

registerData solaredgeRegisters[] = {
    &inverter.wsolar, 1, 40083, 23, SOLAREDGEINVERTER,
    &inverter.wgrid, 1, 40206, 4, SOLAREDGEMETER
};

registerData wibeeeRegisters[] = {
    &inverter.wgrid, 1, 0, 72, WIBEEEMODBUS
};

registerData ingeteamRegisters[] = {
    &inverter.wgrid, 1, 0, 73, INGETEAMMODBUS
};

registerData schneiderRegisters[] = { // Schneider
    &inverter.loadWatts, 201, 98, 66, SCHNEIDERMODBUS1,
    &inverter.wsolar, 201, 354, 4, SCHNEIDERMODBUS2
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

void parseFroniusScale(uint8_t *data) // Optimizar código con un bucle
{
  uint16_t *scalePosition[] = { &froniusVariables.scaleFactorA, &froniusVariables.scaleFactorV, &froniusVariables.scaleFactorW };
  
  int16_t value = 0;

  for (int i = 0; i < sizeOfArray(scalePosition); i++) {
    value = (data[(i * 2)] << 8) | (data[1 + (i * 2)]);

    switch (value)
    {
      case 0:
        *scalePosition[i] = 1.0;
        break;
      case -1:
        *scalePosition[i] = 10.0;
        break;
      case -2:
        *scalePosition[i] = 100.0;
        break;
      case -3:
        *scalePosition[i] = 1000.0;
        break;
    }
  }
}

void parseFroniusPV1(uint8_t *data)
{
  uint16_t value = 0;
  value = (data[0] << 8) | (data[1]);
  if (value != 0xFFFF) inverter.pv1c = (float)value / froniusVariables.scaleFactorA;
  value = (data[2] << 8) | (data[3]);
  if (value != 0xFFFF) inverter.pv1v = (float)value / froniusVariables.scaleFactorV;
  value = (data[4] << 8) | (data[5]);
  if (value != 0xFFFF) inverter.pw1 = (float)value / froniusVariables.scaleFactorW;
}

void parseFroniusPV2(uint8_t *data)
{
  uint16_t value = 0;
  value = (data[0] << 8) | (data[1]);
  if (value != 0xFFFF) inverter.pv2c = (float)value / froniusVariables.scaleFactorA;
  value = (data[2] << 8) | (data[3]);
  if (value != 0xFFFF) inverter.pv2v = (float)value / froniusVariables.scaleFactorV;
  value = (data[4] << 8) | (data[5]);
  if (value != 0xFFFF) inverter.pw2 = (float)value / froniusVariables.scaleFactorW;
}

void parseSunnyBoyGrid(uint8_t *data)
{
  int32_t vertido = 0;
  vertido = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | (data[3]); 
  inverter.wgrid -= (float)vertido;
}

void parseSolarEdgeInverter(uint8_t *data)
{
  int16_t value = 0;
  int16_t SF = 0;

  // Registros 40084 - 40085
  value = (data[0] << 8) | (data[1]);
  SF = (data[2] << 8) | (data[3]);
  inverter.wsolar = value * pow(10, SF);

  // Ignoramos registros 40086 - 40096

  // Registros 40097 - 400102 
  value = (data[26] << 8) | (data[27]);
  SF = (data[28] << 8) | (data[29]);
  inverter.pv1c = value * pow(10, SF);

  value = (data[30] << 8) | (data[31]);
  SF = (data[32] << 8) | (data[33]);
  inverter.pv1v = value * pow(10, SF);

  value = (data[34] << 8) | (data[35]);
  SF = (data[36] << 8) | (data[37]);
  inverter.pw1 = value * pow(10, SF);

  // Ignoramos registro 400103

  // Registros 40104 - 400107
  value = (data[40] << 8) | (data[41]);
  SF = (data[46] << 8) | (data[47]);
  inverter.temperature = value * pow(10, SF);
}

void parseIngeteamModbus(uint8_t *data)
{
  int16_t value = 0;
  uint16_t uvalue = 0;

//   for (int i = 0; i < 69; i++) {
//       INFOV("Registro %d: %02X %02X\n", i + 1, data[(2 * i)], data[(2 * i) + 1]);
//   }

  // Registro 18
  uvalue = (data[34] << 8) | (data[35]);
  meter.voltage = (float)uvalue / 10;

  // Registro 19
  value = (data[36] << 8) | (data[37]);
  meter.current = (float)value / 100;

  // Registro 20
  value = (data[38] << 8) | (data[39]);
  inverter.batteryWatts = (float)value * -1.0;

  // Registro 21
  uvalue = (data[40] << 8) | (data[41]);
  inverter.batterySoC = (float)uvalue;

  // Registro 32
  uvalue = (data[62] << 8) | (data[63]);
  inverter.pv1v = (float)uvalue;

  // Registro 33
  uvalue = (data[64] << 8) | (data[65]);
  inverter.pv1c = (float)uvalue / 100;

  // Registro 34
  uvalue = (data[66] << 8) | (data[67]);
  inverter.pw1 = (float)uvalue;

  // Registro 35
  uvalue = (data[68] << 8) | (data[69]);
  inverter.pv2v = (float)uvalue;

  // Registro 36
  uvalue = (data[70] << 8) | (data[71]);
  inverter.pv2c = (float)uvalue / 100.0;

  // Registro 37
  uvalue = (data[72] << 8) | (data[73]);
  inverter.pw2 = (float)uvalue;

  inverter.wsolar = inverter.pw1 + inverter.pw2;

  // Registro 38
  value = (data[74] << 8) | (data[75]);
  inverter.loadWatts = (float)value;

  // Registro 49
  uvalue = (data[96] << 8) | (data[97]);
  inverter.gridv = (float)uvalue;
  
  // Registro 50

  // Registro 51
  uvalue = (data[100] << 8) | (data[101]);
  meter.frequency = (float)uvalue / 100.0;
  
  // Registro 52
  value = config.flags.useExternalMeter ? (data[142] << 8) | (data[143]) : (data[102] << 8) | (data[103]);
  inverter.wgrid = value;

  config.flags.changeGridSign ? inverter.wgrid *= -1 : inverter.wgrid *= 1;

   // Registro 58
  value = (data[114] << 8) | (data[115]);
  inverter.temperature = value / 10.0;

  // Registro 59 116-117
  // Registro 60 118-119
  // Registro 61 120-121
  // Registro 62 122-123
  // Registro 63 124-125
  // Registro 64 126-127
  // Registro 65 128-129
  // Registro 66 130-131
  // Registro 67 132-133
  // Registro 68 134-135
  // Registro 69 136-137
  // Registro 70 138-139 (Meter)
  // Registro 71 140-141 (Meter)
  // Registro 72 142-143 (Meter) Usado junto registro 52
  // Registro 73 144-145 (Meter)
}

void parseWibeeeModbus(uint8_t *data)
{
  int16_t value = 0;
  uint16_t uvalue = 0;

  // Registro 0
  uvalue = (data[0] << 8) | (data[1]);
  inverter.wgrid = uvalue;
  
  // Registro 2
  uvalue = (data[4] << 8) | (data[5]);
  inverter.wsolar = uvalue;
  
  // Registro 24
  uvalue = (data[48] << 8) | (data[49]);
  meter.importActive = (float)uvalue / 100;
  
  // Registro 48
  uvalue = (data[96] << 8) | (data[97]);
  meter.current = (float)uvalue / 100;
  
  // Registro 58
  uvalue = (data[116] << 8) | (data[117]);
  meter.voltage = (float)uvalue / 100;
  
  // Registro 59
  uvalue = (data[118] << 8) | (data[119]);
  inverter.gridv = (float)uvalue / 100;
  
  // Registro 62
  uvalue = (data[124] << 8) | (data[125]);
  meter.frequency = (float)uvalue / 100;
  
  // Registro 66
  value = (data[132] << 8) | (data[133]);
  meter.powerFactor = (float)value / 100;
  
  if (meter.powerFactor > 0) {
    if (!config.flags.changeGridSign) { inverter.wgrid *= -1; }
  } else {
    meter.powerFactor *= -1;
    if (config.flags.changeGridSign) { inverter.wgrid *= -1; }
  }  
}

void parseSchneiderModbus1(uint8_t *data)
{
  int32_t value = 0;
  uint32_t uvalue = 0;

  // LITTLE ENDIAN COMBOX
  //          DEC HEX
  // Registro 98  (62)
  uvalue = (data[2] << 24) | (data[3] << 16) | (data[0] << 8) | (data[1]);
  inverter.loadWatts = uvalue;

  // Registro 152 (98)
  uvalue = (data[110] << 24) | (data[111] << 16) | (data[108] << 8) | (data[109]);
  meter.voltage = uvalue * 0.001;

  // Registro 154 (9A)
  uvalue = (data[114] << 24) | (data[115] << 16) | (data[112] << 8) | (data[113]);
  inverter.temperature = (uvalue * 0.01) - 273;
  
  // Registro 156 (9B)
  value = (data[118] << 24) | (data[119] << 16) | (data[116] << 8) | (data[117]);
  meter.current = value * 0.001;

  inverter.batteryWatts = meter.voltage * meter.current;
}

void parseSchneiderModbus2(uint8_t *data)
{
  uint32_t PVv = 0;
  uint32_t PVc = 0;

  // LITTLE ENDIAN COMBOX
  //          DEC HEX
  // Registro 354 (162)
  PVv = (data[2] << 24) | (data[3] << 16) | (data[0] << 8) | (data[1]);
  
  // Registro 356 (164)
  PVc = (data[6] << 24) | (data[7] << 16) | (data[4] << 8) | (data[5]);
   
  inverter.wsolar = (PVv * 0.001) * (PVc * 0.001);
}

void parseSolarEdgeMeter(uint8_t *data)
{
  int16_t value = 0;
  int16_t SF = 0;
  value = (data[0] << 8) | (data[1]);
  SF = (data[6] << 8) | (data[7]);
  inverter.wgrid = value * pow(10, SF);
}

float parseFloat32(uint8_t *data, int precision)
{
    float value = 0;
    *((unsigned char *)&value + 3) = data[0];
    *((unsigned char *)&value + 2) = data[1];
    *((unsigned char *)&value + 1) = data[2];
    *((unsigned char *)&value + 0) = data[3];
    // INFOV("Data F32: 0x%.2X 0x%.2X 0x%.2X 0x%.2X\n", data[0], data[1], data[2], data[3]);
    // INFOV("Float 32: %.6f\n", value);

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
    // INFOV("unsigned 16: %" PRIu16 "\n",value);

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
    // INFOV("Data U32: 0x%.2X 0x%.2X 0x%.2X 0x%.2X\n", data[0], data[1], data[2], data[3]);
    // INFOV("Unsigned 32: %" PRIu32 "\n",value);

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
    // INFOV("Data U64 High: 0x%.2X 0x%.2X 0x%.2X 0x%.2X\n", data[0], data[1], data[2], data[3]);
    // INFOV("Data U64 low: 0x%.2X 0x%.2X 0x%.2X 0x%.2X\n", data[4], data[5], data[6], data[7]);
    // INFOV("Unsigned 64: %" PRIu64 "\n",value);

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
    //INFOV("signed 16: %" PRId16 "\n",value);

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
    // INFOV("Data S32: 0x%.2X 0x%.2X 0x%.2X 0x%.2X\n", data[0], data[1], data[2], data[3]);
    // INFOV("Signed 32: %" PRId32 "\n",value);

    if (value == 0x80000000) return 0; // Sanitizer check

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
        // INFOV("Received data address %d\n", a->address);
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
            case FRONIUSSCALE: { parseFroniusScale(data); break; }
            case FRONIUSPV1: { parseFroniusPV1(data); break; }
            case FRONIUSPV2: { parseFroniusPV2(data); break; }
            case SUNNYBOYGRID: { parseSunnyBoyGrid(data); break; }
            case SOLAREDGEINVERTER: { parseSolarEdgeInverter(data); break; }
            case SOLAREDGEMETER: { parseSolarEdgeMeter(data); break; }
            case WIBEEEMODBUS: { parseWibeeeModbus(data); break; }
            case SCHNEIDERMODBUS1: { parseSchneiderModbus1(data); break; }
            case SCHNEIDERMODBUS2: { parseSchneiderModbus2(data); break; }
            case INGETEAMMODBUS: { parseIngeteamModbus(data); break; }
        }
        
        if (config.wversion == FRONIUS_MODBUS) {
          if (a->address == 40097 && !config.flags.changeGridSign) { inverter.wgrid *= -1.0; }
          data_ready = true; 
          froniusVariables.froniusRegisterNum++;
        }

        if (config.wversion == SMA_BOY && a->address == 30867 && !config.flags.changeGridSign) { inverter.wgrid *= -1.0; }
        if (config.wversion == SMA_ISLAND && a->address == 30775 && !config.flags.changeGridSign) { inverter.batteryWatts *= -1.0; }
        if (config.wversion == VICTRON && a->address == 820 && !config.flags.changeGridSign) { inverter.wgrid *= -1.0; }
        if (config.wversion == VICTRON && a->address == 850) { if (!config.flags.useBMV) { inverter.batteryWatts += inverter.wsolar;} } // Custom function for Aeizoon
        if (config.wversion == HUAWEI_MODBUS && a->address == 37113 && config.flags.changeGridSign) { inverter.wgrid *= -1.0; }
        if (config.wversion == SOLAREDGE && a->address == 40206 && config.flags.changeGridSign) { inverter.wgrid *= -1.0; }
        Error.RecepcionDatos = false;
        timers.ErrorRecepcionDatos = millis();
       return;
    });
    modbustcp->onError([](uint16_t packet, esp32Modbus::Error e, void* arg) {
      registerData* a = reinterpret_cast<registerData*>(arg);
      INFOV("Error packet in address %d (%u): %02x\n", a->address, packet, e);
      froniusVariables.froniusRequestSend = false;
    });
}

void checkModbusConnection(uint16_t port)
{
    if (modbustcp == NULL) {
        modbustcp = new esp32ModbusTCP(modbusIP, port);
        configModbusTcp();
    }
}

void smaBoy(void)
{
    checkModbusConnection(502);

    for (uint8_t i = 0; i < sizeOfArray(smaRegistersBoy); ++i) {

        if (modbustcp->readHoldingRegisters(smaRegistersBoy[i].serverID, smaRegistersBoy[i].address, smaRegistersBoy[i].length, &(smaRegistersBoy[i])) > 0) {
            //INFOV("  requested %d\n", smaRegistersBoy[i].address);
        } else {
            INFOV("  error requesting address %d\n", smaRegistersBoy[i].address);
        }
    }
}

void smaIsland(void)
{
    checkModbusConnection(502);

    for (uint8_t i = 0; i < sizeOfArray(smaRegistersIsland); ++i) {

        if (modbustcp->readHoldingRegisters(smaRegistersIsland[i].serverID, smaRegistersIsland[i].address, smaRegistersIsland[i].length, &(smaRegistersIsland[i])) > 0) {
            //INFOV("  requested %d\n", smaRegistersIsland[i].address);
        } else {
            INFOV("  error requesting address %d\n", smaRegistersIsland[i].address);
        }
    }
}

void victron(void)
{
    checkModbusConnection(502);

    for (uint8_t i = 0; i < sizeOfArray(victronRegisters); ++i) {
        if (modbustcp->readHoldingRegisters(victronRegisters[i].serverID, victronRegisters[i].address, victronRegisters[i].length, &(victronRegisters[i])) > 0) {
            // INFOV("  requested %d\n", victronRegisters[i].address);
        } else {
            INFOV("  error requesting address %d\n", victronRegisters[i].address);
        }
    }
}

void fronius(void)
{
    checkModbusConnection(502);

    if (froniusVariables.froniusRegisterNum >= sizeOfArray(froniusRegisters)) { froniusVariables.froniusRegisterNum = 0; }
    if (millis() - froniusVariables.sendTimeOut > 10000) { froniusVariables.froniusRequestSend = false; }

    if (data_ready || !froniusVariables.froniusRequestSend) {
      data_ready = false;
      froniusVariables.sendTimeOut = millis();
      froniusVariables.froniusRequestSend = false;
      if (modbustcp->readHoldingRegisters(froniusRegisters[froniusVariables.froniusRegisterNum].serverID, froniusRegisters[froniusVariables.froniusRegisterNum].address, froniusRegisters[froniusVariables.froniusRegisterNum].length, &(froniusRegisters[froniusVariables.froniusRegisterNum])) > 0) {
        //   INFOV("  requested %d\n", froniusRegisters[froniusVariables.froniusRegisterNum].address);
          froniusVariables.froniusRequestSend = true;
      } else {
          INFOV("  error requesting address %d\n", froniusRegisters[froniusVariables.froniusRegisterNum].address);
          froniusVariables.froniusRequestSend = false;
      }
    }
}

void huawei(void)
{
    checkModbusConnection(502);

    for (uint8_t i = 0; i < sizeOfArray(huaweiRegisters); ++i) {
        if (modbustcp->readHoldingRegisters(huaweiRegisters[i].serverID, huaweiRegisters[i].address, huaweiRegisters[i].length, &(huaweiRegisters[i])) > 0) {
            //INFOV("  requested %d\n", huaweiRegisters[i].address);
        } else {
            INFOV("  error requesting address %d\n", huaweiRegisters[i].address);
        }
    }
    inverter.pw1 = inverter.pv1v * inverter.pv1c;
    inverter.pw2 = inverter.pv2v * inverter.pv2c;
}

void wibeeeModbus(void)
{
    checkModbusConnection(502);

    for (uint8_t i = 0; i < sizeOfArray(wibeeeRegisters); ++i) {
        if (modbustcp->readHoldingRegisters(wibeeeRegisters[i].serverID, wibeeeRegisters[i].address, wibeeeRegisters[i].length, &(wibeeeRegisters[i])) > 0) {
            //INFOV("  requested %d\n", wibeeeRegisters[i].address);
        } else {
            INFOV("  error requesting address %d\n", wibeeeRegisters[i].address);
        }
    }
}

void schneiderModbus(void)
{
    checkModbusConnection(502);

    for (uint8_t i = 0; i < sizeOfArray(schneiderRegisters); ++i) {
        if (modbustcp->readHoldingRegisters(schneiderRegisters[i].serverID, schneiderRegisters[i].address, schneiderRegisters[i].length, &(schneiderRegisters[i])) > 0) {
            // INFOV("  requested %d\n", schneiderRegisters[i].address);
        } else {
            INFOV("  error requesting address %d\n", schneiderRegisters[i].address);
        }
    }
}

void ingeteamModbus(void)
{
    checkModbusConnection(502);

    for (uint8_t i = 0; i < sizeOfArray(ingeteamRegisters); ++i) {
        if (modbustcp->readInputRegisters(ingeteamRegisters[i].serverID, ingeteamRegisters[i].address, ingeteamRegisters[i].length, &(ingeteamRegisters[i])) > 0) {
            // INFOV("  requested %d\n", ingeteamRegisters[i].address);
        } else {
            INFOV("  error requesting address %d\n", ingeteamRegisters[i].address);
        }
    }
}

void solarEdge(void)
{
    checkModbusConnection(1502);

    for (uint8_t i = 0; i < sizeOfArray(solaredgeRegisters); ++i) {
        if (modbustcp->readHoldingRegisters(solaredgeRegisters[i].serverID, solaredgeRegisters[i].address, solaredgeRegisters[i].length, &(solaredgeRegisters[i])) > 0) {
            //INFOV("  requested %d\n", solaredgeRegisters[i].address);
        } else {
            INFOV("  error requesting address %d\n", solaredgeRegisters[i].address);
        }
    }
}