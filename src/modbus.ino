/* MODBUS*/

// METERS STRUCT DEFINITIONS

const uint16_t Ddsu666_start_addresses[] {
  0x2000,   // DDSU666_VOLTAGE             [V]    0
  0x2002,   // DDSU666_CURRENT             [A]    1
  0x2004,   // DDSU666_POWER               [KW]   2
  0x2006,   // DDSU666_REACTIVE_POWER      [KVAR] 3
  0x200A,   // DDSU666_POWER_FACTOR               4
  0x200E,   // DDSU666_FREQUENCY           [Hz]   5
  0X4000,   // DDSU666_IMPORT_ACTIVE       [kWh]  6
  0X400A,   // DDSU666_EXPORT_ACTIVE       [kWh]  7
};

const uint8_t sdm120_table = 8;
const uint8_t sdm220_table = 13;
uint8_t sdm_start_address_count = sdm220_table;

const uint16_t sdm120_start_addresses[] {
  0x0000,   // SDM120C_VOLTAGE             [V]
  0x0006,   // SDM120C_CURRENT             [A]
  0x000C,   // SDM120C_POWER               [W]
  0x0012,   // SDM120C_APPARENT_POWER      [VA]
  0x0018,   // SDM120C_REACTIVE_POWER      [VAR]
  0x001E,   // SDM120C_POWER_FACTOR
  0x0046,   // SDM120C_FREQUENCY           [Hz]
  0x0156,   // SDM120C_TOTAL_ACTIVE_ENERGY [kWh]

  0X0048,   // SDM220_IMPORT_ACTIVE        [kWh]
  0X004A,   // SDM220_EXPORT_ACTIVE        [kWh]
  0X004C,   // SDM220_IMPORT_REACTIVE      [kVArh]
  0X004E,   // SDM220_EXPORT_REACTIVE      [kVArh]
  0X0024    // SDM220_PHASE_ANGLE          [Degree]
};


void ddsu666(void)
{
  bool data_ready = modbusReceiveReady();

  if (data_ready) {
    uint8_t buffer[14];  // At least 5 + (2 * 2) = 9

    uint32_t error = modbusReceiveBuffer(buffer, 2); 

    if (error) {
      INFO("DDSU666 error: ");
      INFOLN(error);
    } else {
     
      //  0  1  2  3  4  5  6  7  8
      // SA FC BC Fh Fl Sh Sl Cl Ch
      // 01 04 04 43 66 33 34 1B 38 = 230.2 Volt
      float value;
      ((uint8_t*)&value)[3] = buffer[3];   // Get float values
      ((uint8_t*)&value)[2] = buffer[4];
      ((uint8_t*)&value)[1] = buffer[5];
      ((uint8_t*)&value)[0] = buffer[6];

      switch(meter.read_state) {
        case 0:
          meter.voltage = value ; // 230.2 V
          break;

        case 1:
          meter.current  = value; // 1.260 A
          break;

        case 2:
          inverter.wgrid = meter.activePower = value * -1000; // -196.3 W
          break;

        case 3:
          meter.reactivePower = value * -1000;   // 92.2
          break;

        case 4:
          meter.powerFactor = value;     // 0.91
          break;

        case 5:
          meter.frequency = value;        // 50.0 Hz
          break;

        case 6:
          meter.importActive = value;    // 478.492 kWh
          break;

        case 7:
          meter.exportActive = value;    // 6.216 kWh
          break;
      }

      meter.read_state++;

      errorLecturaDatos = false;
      errorConexionInversor = false;
      temporizadorErrorConexionRed = millis();
      
      if (meter.read_state == 8) { // 8
        meter.read_state = 0;
      }
    }
  } // end data ready

  if (0 == meter.send_retry || data_ready) {
    meter.send_retry = 5;
    modbusSend(config.idMeter, 0x04, Ddsu666_start_addresses[meter.read_state], 2);
  } else {
    meter.send_retry--;
  }
}

void sdm120(void)
{
  bool data_ready = modbusReceiveReady();

  if (data_ready) {
    uint8_t buffer[14];  // At least 5 + (2 * 2) = 9

    uint32_t error = modbusReceiveBuffer(buffer, 2); 

    if (error) {
      INFO("SDM120 error: ");
      INFOLN(error);
    } else {
     
      //  0  1  2  3  4  5  6  7  8
      // SA FC BC Fh Fl Sh Sl Cl Ch
      // 01 04 04 43 66 33 34 1B 38 = 230.2 Volt
      float value;
      ((uint8_t*)&value)[3] = buffer[3];   // Get float values
      ((uint8_t*)&value)[2] = buffer[4];
      ((uint8_t*)&value)[1] = buffer[5];
      ((uint8_t*)&value)[0] = buffer[6];

      switch(meter.read_state) {
        case 0:
          meter.voltage = value ; // 230.2 V
          break;

        case 1:
          meter.current  = value; // 1.260 A
          break;

        case 2:
          inverter.wgrid = meter.activePower = -value; // -196.3 W
          break;

        case 3:
          meter.aparentPower = -value; // -196.3 W
          break;

        case 4:
          meter.reactivePower = -value;   // 92.2
          break;

        case 5:
          meter.powerFactor = value;     // 0.91
          break;

        case 6:
          meter.frequency = value;        // 50.0 Hz
          break;
        
        case 7:
          meter.energyTotal = value;      // 478.492 kWh import + export
          break;

        case 8:
          meter.importActive = value;    // 478.492 kWh
          break;

        case 9:
          meter.exportActive = value;    // 6.216 kWh
          break;
        
        case 10:
          meter.importReactive = value;    // 478.492 KVArh
          break;

        case 11:
          meter.exportReactive = value;    // 6.216 KVArh
          break;
        
        case 12:
          meter.phaseAngle = value;    // 0.00 Deg
          break;
      }

      meter.read_state++;

      errorLecturaDatos = false;
      errorConexionInversor = false;
      temporizadorErrorConexionRed = millis();
      
      if (meter.read_state == sdm_start_address_count) {
        meter.read_state = 0;

        if (sdm_start_address_count > sdm120_table) {
          if (meter.importActive == -1) { sdm_start_address_count = sdm120_table; }  // No extended registers available
        }
      }
    } // end data ready
  }

  if (0 == meter.send_retry || data_ready) {
    meter.send_retry = 5;
    modbusSend(config.idMeter, 0x04, sdm120_start_addresses[meter.read_state], 2);
  } else {
    meter.send_retry--;
  }
}

void dds2382(void)
{
  bool data_ready = modbusReceiveReady();

  if (data_ready) {
    uint8_t buffer[46];

    uint32_t error = modbusReceiveBuffer(buffer, 18); 
    
    if (error) {
      INFO("DDS238-2 error: ");
      INFOLN(error);
    } else {

      #ifdef FREEDS_DEBUG
        DEBUG("Meter response: ");
        char hexarray[200] = {0};
        char hexvalue[5] = {0};
        for (int i = 0; i < 45; i++)
        {
          sprintf(hexvalue, " 0x%02X", buffer[i]);
          strcpy(hexarray, hexvalue);
          
        }
        DEBUG(hexarray);
      #endif

      float exportActive = 0;
      float importActive = 0;
     
      //           0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F    10    11           = ModBus register
      //  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40  = Buffer index
      // 01 03 24 00 00 1C 7B 00 00 00 00 00 00 00 00 00 00 00 00 00 00 04 F3 00 00 17 88 09 77 01 A6 03 F8 00 70 03 E1 13 8A 63 ED
      // 01 03 24 00 00 1C AD 00 00 00 00 00 00 00 00 00 00 00 00 00 00 04 F3 00 00 17 BA 09 77 01 F1 04 AF 00 6C 03 E3 13 86 41 A2
      // SA FC BC EnergyTotal             ExportActiv ImportActiv                         Volta Curre APowe RPowe PFact Frequ Crc--  = DDS238-2 ZN/S version 1
      // SA FC BC EnergyTotal                                     ExportActiv ImportActiv Volta Curre APowe RPowe PFact Frequ Crc--  = DDS238-2 ZN/S version 2

      meter.energyTotal = (float)((buffer[3] << 24) + (buffer[4] << 16) + (buffer[5] << 8) + buffer[6]) / 100.0;  // 429496.729 kW
      meter.voltage = (float)((buffer[27] << 8) + buffer[28]) / 10;
      meter.current = (float)((buffer[29] << 8) + buffer[30]) / 100.0;
      meter.activePower = inverter.wgrid = -(float)((int16_t)((buffer[31] << 8) + buffer[32])); 
      meter.reactivePower = -((int16_t)((buffer[33] << 8) + buffer[34]));
      meter.powerFactor = (float)((buffer[35] << 8) + buffer[36]) / 1000.0;            // 1.00
      meter.frequency = (float)((buffer[37] << 8) + buffer[38]) / 100.0;               // 50.0 Hz
      
      exportActive = (float)((buffer[11] << 24) + (buffer[12] << 16) + (buffer[13] << 8) + buffer[14]) / 100.0;  // 429496.729 kW
      importActive = (float)((buffer[15] << 24) + (buffer[16] << 16) + (buffer[17] << 8) + buffer[18]) / 100.0;  // 429496.729 kW

      if (importActive > 0 || exportActive > 0) {
               meter.importActive = importActive;
               meter.exportActive = exportActive;
        } else {
               meter.exportActive = (float)((buffer[19] << 24) + (buffer[20] << 16) + (buffer[21] << 8) + buffer[22]) / 100.0;  // 429496.729 kW
               meter.importActive = (float)((buffer[23] << 24) + (buffer[24] << 16) + (buffer[25] << 8) + buffer[26]) / 100.0;  // 429496.729 kW
        }
      
      errorLecturaDatos = false;
      errorConexionInversor = false;
      temporizadorErrorConexionRed = millis();
    }
  } // end data ready

  if (0 == meter.send_retry || data_ready) {
    meter.send_retry = 5;
    modbusSend(config.idMeter, 0x03, 0, 18);
  } else {
    meter.send_retry--;
  }
}

void readMeter(void)
{ 
  DEBUG(F("Baudios: "));
  DEBUGLN(SerieMeter.baudRate());

  switch (config.wversion)
  {
    case 4:
      dds2382();
      break;
    case 5:
      ddsu666();
      break;
    case 6:
      sdm120();
      break;
  }
}
