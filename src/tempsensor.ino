/*
  tempsensor.ino - FreeDs temperature functions
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

void calcDallasTemperature(void)
{
    sensors.setWaitForConversion(true);
    sensors.requestTemperatures();

    float temperatura;

    if (config.termoSensorAddress[0] != 0x0) {
        temperatura = sensors.getTempC(config.termoSensorAddress);

        if (temperatura != -127.0) {
            temperature.temperaturaTermo = temperatura; 
            timers.ErrorLecturaTemperatura[0] = millis();
            Error.temperaturaTermo = false;
        }

        if ((millis() - timers.ErrorLecturaTemperatura[0]) > config.maxErrorTime) {
            Error.temperaturaTermo = true;
            temperature.temperaturaTermo = -127;
            INFOV("Failed to read termo temperature from DS18B20 sensor\n");
        }
    } else { temperature.temperaturaTermo = -127; Error.temperaturaTermo = false;}

    if (config.triacSensorAddress[0] != 0x0) {
        temperatura = sensors.getTempC(config.triacSensorAddress);
        
        if (temperatura != -127.0) {
            temperature.temperaturaTriac = temperatura; 
            timers.ErrorLecturaTemperatura[1] = millis();
            Error.temperaturaTriac = false;
        }

        if ((millis() - timers.ErrorLecturaTemperatura[1]) > config.maxErrorTime) {
            Error.temperaturaTriac = true;
            temperature.temperaturaTriac = -127;
            INFOV("Failed to read triac temperature from DS18B20 sensor\n");
        }
    } else { temperature.temperaturaTriac = -127; Error.temperaturaTriac = false; }

    if (config.customSensorAddress[0] != 0x0) {
        temperatura = sensors.getTempC(config.customSensorAddress);

        if (temperatura != -127.0) {
            temperature.temperaturaCustom = temperatura; 
            timers.ErrorLecturaTemperatura[2] = millis();
            Error.temperaturaCustom = false;
        }

        if ((millis() - timers.ErrorLecturaTemperatura[2]) > config.maxErrorTime) {
            Error.temperaturaCustom = true;
            temperature.temperaturaCustom = -127;
            INFOV("Failed to read termo temperature from DS18B20 sensor\n");
        }
    } else { temperature.temperaturaCustom = -127; Error.temperaturaCustom = false; }
}

void checkTemperature(void)
{
    if (!Error.temperaturaTermo) {
        switch(config.modoTemperatura) {
            case 1: // Auto
                if (!config.flags.pwmMan && temperature.temperaturaTermo < config.temperaturaEncendido) { Flags.tempShutdown = false; Flags.pwmIsWorking = true; }
                if (!config.flags.pwmMan && temperature.temperaturaTermo >= config.temperaturaApagado) { if (pwm.invert_pwm > 0 || (myPID.GetMode() == PID::AUTOMATIC)) { Flags.tempShutdown = true; Flags.pwmIsWorking = false; shutdownPwm(false, "APAGADO TEMP AUTO\n"); } }
                break;
            case 2: // Manual
                if ((config.flags.pwmMan || Flags.pwmManAuto) && temperature.temperaturaTermo < config.temperaturaEncendido) { Flags.tempShutdown = false; Flags.pwmIsWorking = true; }
                if ((config.flags.pwmMan || Flags.pwmManAuto) && temperature.temperaturaTermo >= config.temperaturaApagado) { if (pwm.invert_pwm > 0) { Flags.tempShutdown = true; Flags.pwmIsWorking = false; shutdownPwm(false, "APAGADO TEMP MANUAL\n"); } }
                break;
            case 3: // Auto y Manual
                if (temperature.temperaturaTermo < config.temperaturaEncendido) { Flags.tempShutdown = false; Flags.pwmIsWorking = true; }
                if (temperature.temperaturaTermo >= config.temperaturaApagado) { if (pwm.invert_pwm > 0 || (myPID.GetMode() == PID::AUTOMATIC)) { Flags.tempShutdown = true; Flags.pwmIsWorking = false; shutdownPwm(false, "APAGADO TEMP AUTO/MAN\n"); } }
                break;
        }
    }

    if (((millis() - timers.ErrorLecturaTemperatura[0]) > config.maxErrorTime) && config.modoTemperatura > 0 && config.termoSensorAddress[0] != 0x0)
    {
        if (pwm.invert_pwm > 0) { Flags.pwmIsWorking = false; shutdownPwm(false, "APAGADO TEMP ERROR LECTURA\n"); }
    }
}

void buildSensorArray(void)
{
    uint8_t idCount = 0;

    while (oneWire.search(temperature.tempSensorAddress[idCount])) {
        INFOV("DS18B20 ID %i: 0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X\n", idCount + 1, temperature.tempSensorAddress[idCount][0], temperature.tempSensorAddress[idCount][1], temperature.tempSensorAddress[idCount][2], temperature.tempSensorAddress[idCount][3], temperature.tempSensorAddress[idCount][4], temperature.tempSensorAddress[idCount][5], temperature.tempSensorAddress[idCount][6], temperature.tempSensorAddress[idCount][7]);
        idCount++;
    }  
        
}