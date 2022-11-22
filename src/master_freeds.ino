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
  if (config.flags.messageDebug) { INFOV("Size: %d, Json: %s\n", strlen(json), json); }

  DeserializationError error = deserializeJson(root, json);
  
  if (error) {
    INFOV("deserializeJson() failed: %s\n", error.c_str());
  } else {
    masterMode = (int)root["wversion"];
    inverter.wgrid =  (float)root["wgrid"]; // Potencia de red
      
    if ((int)root["PwmMaster"] >= config.pwmSlaveOn) {
      Flags.pwmIsWorking = true;
    } else {
      if (!config.flags.pwmMan && invert_pwm > 0) {
        Flags.pwmIsWorking = false;
        down_pwm(false);
      }
    }

    switch (masterMode)
    {
      case 3: // Mqtt
        inverter.pv1c =   (float)root["pv1c"]; // Corriente string 1
        inverter.pv2c =   (float)root["pv2c"]; // Corriente string 2
        inverter.pv1v =   (float)root["pv1v"]; // Tension string 1
        inverter.pv2v =   (float)root["pv2v"]; // Tension string 2
        inverter.pw1 =    (float)root["pw1"];    // Potencia string 1
        inverter.pw2 =    (float)root["pw2"];    // Potencia string 2
        inverter.gridv =  (float)root["gridv"];   // Tension de red
        inverter.wtoday = (float)root["wtoday"];    // Potencia solar diaria
        inverter.wsolar = (float)root["wsolar"];    // Potencia solar actual
        inverter.temperature =  (float)root["invTemp"];
        break;
      case 4:
      case 5:
      case 6:
        meter.voltage =      (float)root["mvoltage"];
        meter.current =      (float)root["mcurrent"];
        meter.powerFactor =  (float)root["mpowerFactor"];
        meter.frequency =    (float)root["mfrequency"];
        meter.importActive = (float)root["mimportActive"];
        meter.exportActive = (float)root["mexportActive"];
        meter.energyTotal =  (float)root["menergyTotal"];
        meter.activePower =  (float)root["mactivePower"];
        meter.aparentPower = (float)root["maparentPower"];
        meter.reactivePower =  (float)root["mreactivePower"];
        meter.importReactive = (float)root["mimportReactive"];
        meter.exportReactive = (float)root["mexportReactive"];
        meter.phaseAngle =     (float)root["mphaseAngle"];
        break;
      case 9:
      case 10:
        meter.voltage =       (float)root["mvoltage"];
        meter.powerFactor =   (float)root["mpowerFactor"];
        meter.importActive =  (float)root["mimportActive"];
        meter.exportActive =  (float)root["mexportActive"];
        meter.activePower =   (float)root["mactivePower"];
        meter.reactivePower = (float)root["mreactivePower"];
        inverter.wsolar =     (float)root["wsolar"];
        inverter.gridv =      (float)root["gridv"];
        break;
      case 13:
        meter.voltage =         (float)root["mvoltage"];
        meter.current =         (float)root["mcurrent"];
        inverter.pv1c =         (float)root["pv1c"]; // Corriente string 1
        inverter.pv2c =         (float)root["pv2c"]; // Corriente string 2
        inverter.pv1v =         (float)root["pv1v"]; // Tension string 1
        inverter.pv2v =         (float)root["pv2v"]; // Tension string 2
        inverter.pw1 =          (float)root["pw1"];    // Potencia string 1
        inverter.pw2 =          (float)root["pw2"];    // Potencia string 2
        inverter.gridv =        (float)root["gridv"];   // Tension de red
        inverter.wtoday =       (float)root["wtoday"];    // Potencia solar diaria
        inverter.wsolar =       (float)root["wsolar"];    // Potencia solar actual
        inverter.batteryWatts = (float)root["wbattery"];
        inverter.loadWatts =    (float)root["wload"];
        inverter.temperature =  (float)root["invTemp"];
        break;
      case 14:
        meter.voltage =         (float)root["mvoltage"];
        meter.current =         (float)root["mcurrent"];
        inverter.wsolar =       (float)root["wsolar"];
        inverter.batteryWatts = (float)root["wbattery"];
        inverter.batterySoC =   (float)root["invSoC"];
        inverter.temperature =  (float)root["invTemp"];
        break;
      case 15:
        meter.voltage =   (float)root["mvoltage"];
        meter.current =   (float)root["mcurrent"];
        inverter.pv1c =   (float)root["pv1c"]; // Corriente string 1
        inverter.pv2c =   (float)root["pv2c"]; // Corriente string 2
        inverter.pv1v =   (float)root["pv1v"]; // Tension string 1
        inverter.pv2v =   (float)root["pv2v"]; // Tension string 2
        inverter.pw1 =    (float)root["pw1"];    // Potencia string 1
        inverter.pw2 =    (float)root["pw2"];    // Potencia string 2
        inverter.wtoday = (float)root["wtoday"];    // Potencia solar diaria
        inverter.wsolar = (float)root["wsolar"];    // Potencia solar actual
        inverter.temperature =  (float)root["invTemp"];
        break;
      default:
        inverter.wtoday = (float)root["wtoday"];
        inverter.wsolar = (float)root["wsolar"];
        inverter.gridv =  (float)root["gridv"];
        inverter.pv1c =   (float)root["pv1c"];
        inverter.pv1v =   (float)root["pv1v"];
        inverter.pw1 =    (float)root["pw1"];
        inverter.pv2c =   (float)root["pv2c"];
        inverter.pv2v =   (float)root["pv2v"];
        inverter.pw2 =    (float)root["pw2"];
        inverter.temperature =  (float)root["invTemp"];
        break;
    }
    Error.RecepcionDatos = false;
    timers.ErrorRecepcionDatos = millis();
  }
}