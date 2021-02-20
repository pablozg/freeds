/*
  master_freeds.ino - Master Freeds Support
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

void parseMasterFreeDs(char *json)
{
  if (config.flags.debug3) { INFOV("Size: %d, Json: %s\n", strlen(json), json); }

  DeserializationError error = deserializeJson(root, json);
  
  if (error) {
    INFOV("deserializeJson() failed: %s\n", error.c_str());
  } else {
    slave.masterMode = (int)root["wversion"];
    
    slave.masterPwmValue = (bool)root["tempShutdown"] ? 100 : (int)root["PwmMaster"];

    if (slave.masterPwmValue >= config.pwmSlaveOn) {
      Flags.pwmIsWorking = true;
    } else {
      if (!config.flags.pwmMan && pwm.invert_pwm > 0) {
        Flags.pwmIsWorking = false;
        shutdownPwm(true, "PWM: disabled by low % on master");
      }
    }

  defineWebMonitorFields(slave.masterMode);
  // Inverter data
  if (webMonitorFields.wsolar) {
    inverter.wsolar = (float)root["wsolar"]; // Potencia solar actual
  }
  if (webMonitorFields.wgrid) {
    inverter.wgrid = (float)root["wgrid"]; // Potencia de red
    if (config.flags.changeGridSign) { inverter.wgrid *= -1.0; }
  }
  if (webMonitorFields.temperature) {
    inverter.temperature = (float)root["invTemp"]; // Temperatura Inversor
  }
  if (webMonitorFields.batteryWatts) {
    inverter.batteryWatts = (float)root["wbattery"];
  }
  if (webMonitorFields.batterySoC) {
    inverter.batterySoC = (float)root["invSoC"];
  }
  if (webMonitorFields.loadWatts) {
    inverter.loadWatts = (float)root["wload"];
  }
  if (webMonitorFields.wtoday) {
    inverter.wtoday = (float)root["wtoday"]; // Potencia solar diaria
  }
  if (webMonitorFields.gridv) {
    inverter.gridv = (float)root["gridv"]; // Tension de red
  }
  if (webMonitorFields.pv1c) {
    inverter.pv1c = (float)root["pv1c"]; // Corriente string 1
  }
  if (webMonitorFields.pv1v) {
    inverter.pv1v = (float)root["pv1v"]; // Tension string 1
  }
  if (webMonitorFields.pw1) {
    inverter.pw1 = (float)root["pw1"]; // Potencia string 1
  }
  if (webMonitorFields.pv2c) {
    inverter.pv2c = (float)root["pv2c"]; // Corriente string 2
  }
  if (webMonitorFields.pv2v) {
    inverter.pv2v = (float)root["pv2v"]; // Tension string 2
  }
  if (webMonitorFields.pw2) {
    inverter.pw2 = (float)root["pw2"]; // Potencia string 2  
  }

  // Meter data
  if (webMonitorFields.voltage) {
    meter.voltage = (float)root["mvoltage"];
  }
  if (webMonitorFields.current) {
    meter.current = (float)root["mcurrent"];
  }
  if (webMonitorFields.powerFactor) {
    meter.powerFactor = (float)root["mpowerFactor"];
  }
  if (webMonitorFields.frequency) {
    meter.frequency = (float)root["mfrequency"];
  }
  if (webMonitorFields.importActive) {
    meter.importActive = (float)root["mimportActive"];
  }
  if (webMonitorFields.exportActive) {
    meter.exportActive = (float)root["mexportActive"];
  }

    Error.RecepcionDatos = false;
    timers.ErrorRecepcionDatos = millis();
  }
}