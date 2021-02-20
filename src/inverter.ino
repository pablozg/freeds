/*
  inverter.ino - FreeDs inverter functions
  Derivador de excedentes para ESP32 DEV Kit // Wifi Kit 32

  Inspired in opends+ (https://github.com/iqas/derivador)
  
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

// Solax v2
void readESP01(void)
{
  if (config.flags.debug2) { INFOV("readESP01()\n"); }
  
  if (SerieEsp.available())
  {
    String Data = SerieEsp.readStringUntil('\n');
    if (Data.startsWith("{\"Data\":")) {
      parseJson(Data);
    }
    INFOV("Datos EPS01: %s\n", Data.c_str());
  }
}

// Solax v2 (Serial ESP01)
void parseJson(String json)
{
  if (config.flags.debug3) { INFOV("Size: %d, Json: %s\n", strlen(json.c_str()), json.c_str()); }
  
  DeserializationError error = deserializeJson(root, json);
  
  if (error)  {
    INFOV("deserializeJson() failed: %s\n", error.c_str());
  } else {
    
    inverter.pv1c = root["Data"][0];     // Corriente string 1
    inverter.pv2c = root["Data"][1];     // Corriente string 2
    inverter.pv1v = root["Data"][2];     // Tension string 1
    inverter.pv2v = root["Data"][3];     // Tension string 2
    inverter.pw1 = root["Data"][4];      // Potencia string 1
    inverter.pw2 = root["Data"][5];      // Potencia string 2
    inverter.gridv = root["Data"][6];    // Tension de red
    inverter.wsolar = root["Data"][7];   // Potencia solar
    inverter.temperature = root["Data"][8];   // Potencia temperature
    inverter.wtoday = root["Data"][9];   // Potencia solar diaria
    inverter.wgrid = root["Data"][10];    // Potencia de red (Negativo: de red - Positivo: a red)
    inverter.wtogrid = root["Data"][11]; // Potencia diaria enviada a red
    meter.voltage = root["Data"][12];
    meter.current = root["Data"][13];
    inverter.batteryWatts = root["Data"][14];
    inverter.batterySoC = root["Data"][15];

    if (config.flags.changeGridSign) { inverter.wgrid *= -1.0; }

    Error.RecepcionDatos = false;
    timers.ErrorRecepcionDatos = millis();
  }
}

// Solax v1
void parseJsonv1(char *json)
{
  char newv1[380];
  uint16_t copyPos = 0;

  for (int i = 0; i < strlen(json); i++)
  {
      newv1[copyPos]=json[i];
      copyPos++;
      if (json[i] == ',' && json[i+1] == ',') { newv1[copyPos]= '0'; copyPos++; }
  }
  newv1[copyPos]= '\0';

  if (config.flags.debug3) { INFOV("Size: %d, Json: %s\n", strlen(newv1), newv1); }

  DeserializationError error = deserializeJson(root, newv1);
  
  if (error) {
    INFOV("deserializeJson() failed: %s\n", error.c_str());
  } else {

    inverter.pv1c = root["Data"][0];     // Corriente string 1
    inverter.pv2c = root["Data"][1];     // Corriente string 2
    inverter.pv1v = root["Data"][2];     // Tension string 1
    inverter.pv2v = root["Data"][3];     // Tension string 2
    inverter.gridv = root["Data"][5];    // Tension de red
    inverter.wsolar = root["Data"][6];   // Potencia solar
    inverter.temperature = (float)root["Data"][7] / 100.0;   // Temperatura
    inverter.wtoday = root["Data"][8];   // Potencia solar diaria
    inverter.wgrid = root["Data"][10];   // Potencia de red (Negativo: de red - Positivo: a red)
    inverter.pw1 = root["Data"][11];     // Potencia string 1
    inverter.pw2 = root["Data"][12];     // Potencia string 2
    meter.voltage = root["Data"][13];
    meter.current = root["Data"][14];
    inverter.batteryWatts = root["Data"][15];
    inverter.batterySoC = root["Data"][17];
    inverter.wtogrid = root["Data"][41]; // Potencia diaria enviada a red

    if (config.flags.changeGridSign) { inverter.wgrid *= -1.0; }

    Error.RecepcionDatos = false;
    timers.ErrorRecepcionDatos = millis();
  }
}

// Solax v2 local
void parseJsonv2local(char *json)
{
  if (config.flags.debug3) { INFOV("Size: %d, Json: %s\n", strlen(json), json); }
  
  DeserializationError error = deserializeJson(root, json);
  
  if (error) {
    INFOV("deserializeJson() failed: %s\n", error.c_str());
  } else {

    inverter.pv1c = root["Data"][0];     // Corriente string 1
    inverter.pv2c = root["Data"][1];     // Corriente string 2
    inverter.pv1v = root["Data"][2];     // Tension string 1
    inverter.pv2v = root["Data"][3];     // Tension string 2
    inverter.gridv = root["Data"][5];    // Tension de red
    inverter.wsolar = root["Data"][6];   // Potencia solar
    inverter.temperature = root["Data"][7];   // Temperatura
    inverter.wtoday = root["Data"][8];   // Potencia solar diaria
    inverter.wgrid = root["Data"][10];   // Potencia de red (Negativo: de red - Positivo: a red)
    inverter.pw1 = root["Data"][11];     // Potencia string 1
    inverter.pw2 = root["Data"][12];     // Potencia string 2
    meter.voltage = root["Data"][13];
    meter.current = root["Data"][14];
    inverter.batteryWatts = root["Data"][15];
    inverter.batterySoC = root["Data"][17];
    inverter.wtogrid = root["Data"][41]; // Potencia diaria enviada a red

    if (config.flags.changeGridSign) { inverter.wgrid *= -1.0; }

    Error.RecepcionDatos = false;
    timers.ErrorRecepcionDatos = millis();
  }
}

void parseJsonv3local(String json)
{
  DeserializationError error = deserializeJson(root, json);

  if (error) {
    Serial.printf("deserializeJson() failed: %s\n", error.c_str());
  } else {
    inverter.gridv = (float)root["Data"][0] / 10.0;   // Tension de red
    inverter.wsolar = (float)root["Data"][2];         // Potencia solar
    inverter.pv1v = (float)root["Data"][3] / 10.0;    // Tension string 1
    inverter.pv2v = (float)root["Data"][4] / 10.0;    // Tension string 2
    inverter.pv1c = (float)root["Data"][5] / 10.0;    // Corriente string 1
    inverter.pv2c = (float)root["Data"][6] / 10.0;    // Corriente string 2
    inverter.pw1 = (float)root["Data"][7];            // Potencia string 1
    inverter.pw2 = (float)root["Data"][8];            // Potencia string 2
    inverter.wtoday = (float)root["Data"][13] / 10.0; // Potencia solar diaria
    inverter.temperature = (float)root["Data"][39];   // Temperatura
    inverter.wgrid = (float)root["Data"][48] - (float)root["Data"][49];   // Potencia de red (Negativo: de red - Positivo: a red)
    //inverter.wtogrid = 0; // Potencia diaria enviada a red

    // meter.voltage = root["Data"][13];
    // meter.current = root["Data"][14];
    // inverter.batteryWatts = root["Data"][15];
    // inverter.batterySoC = root["Data"][17];

    if (config.flags.changeGridSign) { inverter.wgrid *= -1.0; }

    Error.RecepcionDatos = false;
    timers.ErrorRecepcionDatos = millis();
  }
}

// Fronius
void parseJsonFronius(char *json)
{
  if (config.flags.debug3) { INFOV("Size: %d, Json: %s\n", strlen(json), json); }
  DeserializationError error = deserializeJson(root, json);
  
  if (error) {
    INFOV("deserializeJson() failed: %s\n", error.c_str());
  } else {
    inverter.wsolar = root["Body"]["Data"]["Site"]["P_PV"] == "null" ? 0 : root["Body"]["Data"]["Site"]["P_PV"];     // Potencia solar
    inverter.wtoday = root["Body"]["Data"]["Site"]["E_Day"] == "null" ? 0 : root["Body"]["Data"]["Site"]["E_Day"];   // Potencia solar diaria
    inverter.wgrid  = root["Body"]["Data"]["Site"]["P_Grid"] == "null" ? 0 : root["Body"]["Data"]["Site"]["P_Grid"]; // Potencia de red (Negativo: de red - Positivo: a red)
    if (!config.flags.changeGridSign) { inverter.wgrid *= -1.0; }
    inverter.wtoday = inverter.wtoday / 1000; // w->Kw
    
    Error.RecepcionDatos = false;
    timers.ErrorRecepcionDatos = millis();
  }
}