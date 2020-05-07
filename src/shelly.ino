/*
  shelly.ino - FreeDs Shelly Support
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

// Shelly EM
void parseShellyEM(String json, int sensor)
{
  if (config.flags.debugOutput) { INFOV("Size: %lu, Json: %s\n", strlen(json.c_str()), json.c_str()); }
  
  DeserializationError error = deserializeJson(root, json);
  
  if (error) {
    INFOV("deserializeJson() failed: %s\n", error.c_str());
    httpcode = -1;
  } else {
    switch (sensor)
    {
      case 1: // Medida de Red
        if (root["is_valid"] == true) {
          meter.activePower = inverter.wgrid = roundf((float)root["power"] * -1.0); // Potencia de red (Negativo: de red - Positivo: a red)
          meter.voltage = roundf((float)root["voltage"]);
          meter.reactivePower = roundf((float)root["reactive"] * -1.0);
          meter.importActive = roundf((float)root["total"] / 1000.0);
          meter.exportActive = roundf((float)root["total_returned"] / 1000.0);
          Error.ConexionInversor = false;
        }
        break;
      case 2: // Medida de Inversor
        if (root["is_valid"] == true) {
          inverter.wsolar = roundf((float)root["power"]); // Potencia de red (Negativo: de red - Positivo: a red)
          inverter.gridv = roundf((float)root["voltage"]);
          Error.ConexionInversor = false;
        }
        break;
    }
    timers.ErrorConexionRed = millis();
  }
}