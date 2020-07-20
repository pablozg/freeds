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
    // unsigned long start = millis();
    sensors.setWaitForConversion(true);
    sensors.requestTemperatures();
    delay(30); // To ensure a completed conversion

    if (config.termoSensorAddress[0] != 0x0) {
        temperaturaTermo = sensors.getTempC(config.termoSensorAddress);
        // Serial.printf("Time used: %lu\n", millis() - start);
        if (temperaturaTermo == -127.0) { Error.temperaturaTermo = true; INFOV("Failed to read termo temperature from DS18B20 sensor\n"); } else { Error.temperaturaTermo = false; }
    } else { temperaturaTermo = -127.0; Error.temperaturaTermo = false;}

    if (config.triacSensorAddress[0] != 0x0) {
        temperaturaTriac = sensors.getTempC(config.triacSensorAddress);
        if (temperaturaTriac == -127.0) { Error.temperaturaTriac = true; INFOV("Failed to read triac temperature from DS18B20 sensor\n"); } else { Error.temperaturaTriac = false; }
    } else { temperaturaTriac = -127.0; Error.temperaturaTriac = false; }

    if (config.customSensorAddress[0] != 0x0) {
        temperaturaCustom = sensors.getTempC(config.customSensorAddress);
        if (temperaturaCustom == -127.0) { Error.temperaturaCustom = true; INFOV("Failed to read temperature from DS18B20 custom sensor\n"); } else { Error.temperaturaCustom = false; }
    } else { temperaturaCustom = -127.0; Error.temperaturaCustom = false; }
}

void checkTemperature(void)
{
    if (config.flags.sensorTemperatura && temperaturaTermo != -127.00 && !Error.temperaturaTermo) {
        timers.ErrorLecturaTemperatura = millis();
        switch(config.modoTemperatura) {
            case 1:
                if (!config.flags.pwmMan && temperaturaTermo < config.temperaturaEncendido) { Flags.pwmIsWorking = true; }
                if (!config.flags.pwmMan && temperaturaTermo >= config.temperaturaApagado) { if (invert_pwm > 0) { Flags.pwmIsWorking = false; down_pwm(false); } }
                break;
            case 2:
                if ((config.flags.pwmMan || Flags.pwmManAuto) && temperaturaTermo < config.temperaturaEncendido) { Flags.pwmIsWorking = true; }
                if ((config.flags.pwmMan || Flags.pwmManAuto) && temperaturaTermo >= config.temperaturaApagado) { if (invert_pwm > 0) { Flags.pwmIsWorking = false; down_pwm(false); } }
                break;
            case 3:
                if (temperaturaTermo < config.temperaturaEncendido) { Flags.pwmIsWorking = true; }
                if (temperaturaTermo >= config.temperaturaApagado) { if (invert_pwm > 0) { Flags.pwmIsWorking = false; down_pwm(false); } }
                break;
        }
    }

    if (((millis() - timers.ErrorLecturaTemperatura) > config.maxErrorTime) && config.modoTemperatura > 0)
    {
        if (invert_pwm > 0) { Flags.pwmIsWorking = false; down_pwm(false); }
    }
}

void buildSensorArray(void)
{
    uint8_t idCount = 0;

    while (oneWire.search(tempSensorAddress[idCount])) {
        INFOV("DS18B20 ID %i: 0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X 0x%.2X\n", idCount + 1, tempSensorAddress[idCount][0], tempSensorAddress[idCount][1], tempSensorAddress[idCount][2], tempSensorAddress[idCount][3], tempSensorAddress[idCount][4], tempSensorAddress[idCount][5], tempSensorAddress[idCount][6], tempSensorAddress[idCount][7]);
        idCount++;
    }  
        
}