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
void parseShellyEM(char *json, int sensor)
{
  if (config.flags.debug3) { INFOV("Shelly Size: %d, Json: %s\n", strlen(json), json); }
  
  DeserializationError error = deserializeJson(root, json);
  
  if (error) {
    INFOV("deserializeJson() failed: %s\n", error.c_str());
  } else {
    switch (sensor)
    {
      case 1: // Medida de Red
        if (root["is_valid"] == true) {
          meter.activePower = inverter.wgrid = roundf((float)root["power"]);
          meter.voltage = inverter.gridv = roundf((float)root["voltage"]);
          meter.reactivePower = roundf((float)root["reactive"]);
          meter.importActive = roundf((float)root["total"] / 1000.0);
          meter.exportActive = roundf((float)root["total_returned"] / 1000.0);
          Error.RecepcionDatos = false;
          if (!config.flags.changeGridSign) { meter.activePower *= -1.0; inverter.wgrid *= -1.0; meter.reactivePower *= -1.0;}
        }
        break;
      case 2: // Medida de Inversor
        if (root["is_valid"] == true) {
          inverter.wsolar = roundf((float)root["power"]); // Potencia de red (Negativo: de red - Positivo: a red)
          // inverter.gridv = roundf((float)root["voltage"]);
          Error.RecepcionDatos = false;
        }
        break;
    }
    timers.ErrorRecepcionDatos = millis();
  }
}