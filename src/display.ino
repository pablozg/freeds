/*
  display.ino - Display Support
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

void showOledData(void)
{
  //if (config.flags.debug1 == 4) { INFOV("showOledData()\n"); }
#ifdef OLED
  uint8_t wversion = 0;

  if ((millis() - timers.FlashDisplay) > 1000)
  {
    timers.FlashDisplay = millis();
    Flags.flash = !Flags.flash;
  }

  if (config.wversion == SLAVE_MODE) { wversion = slave.masterMode; }
  else { wversion = config.wversion; }

  if (config.flags.wifi)
  {
    switch (button.screen)
    {
      case 0: // Principal
        display.clear();

        // Texto Columnas
        display.setFont(ArialMT_Plain_10);

        // Columna Izquierda
        display.setTextAlignment(TEXT_ALIGN_LEFT);
        switch (wversion)
        {
          case DDS238_METER:
          case DDSU666_METER:
          case SDM_METER:
            display.drawString(0, 0, (Flags.flash ? lang._VOLTAGE_ : lang._CURRENT_));
            break;
          case VICTRON:
          case SMA_ISLAND:
          case SCHNEIDER:
            display.drawString(0, 0, lang._BATTERY_);
            break;
          default:
            display.drawString(0, 0, lang._SOLAR_);
            break;
        }

        // Columna Derecha
        display.setTextAlignment(TEXT_ALIGN_RIGHT);
        switch (wversion)
        {
          case SMA_ISLAND:
            display.drawString(128, 0, "SoC");
            break;
          case SCHNEIDER:
            display.drawString(128, 0, "Volts");
            break;
          default:
            display.drawString(128, 0, lang._GRID_);
            break;
        }

        // Datos Columnas
        display.setFont(ArialMT_Plain_24);

        // Columna Izquierda
        display.setTextAlignment(TEXT_ALIGN_LEFT);
        switch (wversion)
        {
          case DDS238_METER:
          case DDSU666_METER:
          case SDM_METER:
            display.setFont(ArialMT_Plain_16);
            display.drawString(0, 14, (Flags.flash ? String(meter.voltage) : String(meter.current)));
            break;
          case VICTRON:
          case SMA_ISLAND:
          case SCHNEIDER:
            display.drawString(0, 12, (String)(int)inverter.batteryWatts);
            break;
          default:
            display.drawString(0, 12, (String)(int)inverter.wsolar);
            break;
        }

        // Columna Derecha
        display.setTextAlignment(TEXT_ALIGN_RIGHT);
        switch (wversion)
        {
          case SMA_ISLAND:
            display.drawString(128, 12, (String)(int)inverter.batterySoC + "%");
            break;
          case SCHNEIDER:
            display.drawString(128, 12, (String)(int)meter.voltage);
            break;
          default:
            display.drawString(128, 12, (String)(int)inverter.wgrid);
            break;
        }

        display.setFont(ArialMT_Plain_10);
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        // Display Select Mode
        if (config.wversion == SLAVE_MODE) { display.drawString(69, 0, "SLV"); }
        else { 
          switch (wversion)
          {
            case SOLAX_V2_LOCAL:
              display.drawString(69, 0, "SV2L");
              break;
            case SOLAX_V1:
              display.drawString(69, 0, "SV1");
              break;
            case SOLAX_V2:
              display.drawString(69, 0, "SV2");
              break;
            case MQTT_BROKER:
              display.drawString(69, 0, "MQTT");
              break;
            case DDS238_METER:
              display.drawString(69, 0, "m238");
              break;
            case DDSU666_METER:
              display.drawString(69, 0, "m666");
              break;
            case SDM_METER:
              display.drawString(69, 0, "mSDM");
              break;
            case MUSTSOLAR:
              display.drawString(69, 0, "MSTS");
              break;
            case GOODWE:
              display.drawString(69, 0, "GDWE");
              break;
            case SMA_BOY:
              display.drawString(69, 0, "SMAB");
              break;
            case SMA_ISLAND:
              display.drawString(69, 0, "SMAI");
              break;
            case WIBEEE:
              display.drawString(69, 0, "WIBE");
              break;
            case SHELLY_EM:
              display.drawString(69, 0, "SHLY");
              break;
            case FRONIUS_API:
              display.drawString(69, 0, "FAPI");
              break;
            case ICC_SOLAR:
              display.drawString(69, 0, "ICCS");
              break;
            case VICTRON:
              display.drawString(69, 0, "VICT");
              break;
            case FRONIUS_MODBUS:
              display.drawString(69, 0, "FBUS");
              break;
            case HUAWEI_MODBUS:
              display.drawString(69, 0, "HWEI");
              break;
            case SCHNEIDER:
              display.drawString(69, 0, "SCHN");
              break;
          }
        }

        if (Flags.Updating)
          display.drawString(64, 38, lang._UPDATING_);
        else if (Error.ConexionWifi)
          display.drawString(64, 38, lang._LOSTWIFI_);
        // else if ((!config.flags.pwmEnabled || (!config.flags.pwmMan && (Error.VariacionDatos || Error.RecepcionDatos))) && pwm.invert_pwm <= 1)
        else if ((!config.flags.pwmMan && (Error.VariacionDatos || Error.RecepcionDatos)) && pwm.invert_pwm <= 1)
          display.drawString(64, 38, WiFi.localIP().toString());
        else {
          display.drawProgressBar(0, 38, 127, 12, pwm.pwmValue); // draw the progress bar

          display.setTextAlignment(TEXT_ALIGN_CENTER);
          if (config.flags.pwmEnabled == false) { display.drawString(64, 38, "PWM: OFF"); }
          else {
            display.setColor(INVERSE);
            if (config.wversion == SLAVE_MODE) {
              display.drawString(64, 38, (config.flags.pwmMan ? "PWM: " + String(pwm.pwmValue) + "% (MANUAL)" : "MSTR: " + String(slave.masterPwmValue) + "%" + " PWM: " + String(pwm.pwmValue) + "%"));
            } else {
              display.drawString(64, 38, (config.flags.pwmMan ? "PWM: " + String(pwm.pwmValue) + "% (MANUAL)" : "PWM: " + String(pwm.pwmValue) + "%"));
            }
            display.setColor(WHITE);
          }
        }

        display.setTextAlignment(TEXT_ALIGN_LEFT);
        if (Flags.flash)
        {
          display.drawString(5, 52, (String(Error.RecepcionDatos ? "S" : "S")));
          display.drawString(17, 52, (String(Error.ConexionWifi ? "W" : "W")));
          display.drawString(30, 52, (String(Error.ConexionMqtt ? "M" : "M")));
        }
        else
        {
          display.drawString(5, 52, (String(Error.RecepcionDatos ? "_" : "S")));
          display.drawString(17, 52, (String(Error.ConexionWifi ? "_" : "W")));
          display.drawString(30, 52, (String(Error.ConexionMqtt ? "_" : "M")));

        }
        display.setTextAlignment(TEXT_ALIGN_RIGHT);
        display.drawString(128, 52, (lang._RELAY_ + String((digitalRead(PIN_RL1) ? "1 " : "_ ")) + String((digitalRead(PIN_RL2) ? "2 " : "_ ")) + String((digitalRead(PIN_RL3) ? "3 " : "_ ")) + String((digitalRead(PIN_RL4) ? "4 " : "_ "))));

        display.display();
        break;

      case 1: // Strings Info
          if (wversion < DDS238_METER || wversion > SDM_METER) {
            display.clear();
            display.setFont(ArialMT_Plain_10);
            display.setTextAlignment(TEXT_ALIGN_CENTER);
            display.drawString(64, 0, lang. _INVERTERINFO_);
            display.drawString(19, 12, lang._OLEDPOWER_);
            display.drawString(60, 12, lang._GRID_);
            display.drawString(102, 12, lang._OLEDTODAY_);
            display.drawString(19, 22, (String(int(inverter.wsolar)) + "W"));
            display.drawString(60, 22, (String(int(inverter.wgrid)) + "W"));
            display.drawString(102, 22, String(inverter.wtoday) + "Kw");
            display.drawString(30, 34, "STRING 1");
            display.drawString(30, 44, (String(int(inverter.pw1)) + "W"));
            display.drawString(30,  54, (String(int(inverter.pv1v)) + "V " + String(inverter.pv1c) + "A"));
            display.drawString(100, 34, "STRING 2");
            display.drawString(100, 44, (String(int(inverter.pw2)) + "W"));
            display.drawString(100,  54, (String(int(inverter.pv2v)) + "V " + String(inverter.pv2c) + "A"));
            display.display();
          } else { button.screen++; }
          break;
      
      case 2: // Meters
          if (wversion >= DDS238_METER && wversion <= SDM_METER) {
            display.clear();
            display.setFont(ArialMT_Plain_10);
            display.setTextAlignment(TEXT_ALIGN_CENTER);
            display.drawString(64, 0, lang. _METERINFO_);
            display.drawString(19, 12, lang._OLEDPOWER_);
            display.drawString(60, 12, lang._VOLTAGE_);
            display.drawString(102, 12, lang._CURRENT_);
            display.drawString(19, 22, (String(int(meter.activePower)) + "W"));
            display.drawString(60, 22, (String(int(meter.voltage)) + "V"));
            display.drawString(102, 22, String(meter.current) + "A");
            display.drawString(30, 34, lang._IMPORT_);
            display.drawString(30, 44, (String(meter.importActive) + "KWH"));
            display.drawString(100, 34, lang._EXPORT_);
            display.drawString(100, 44, (String(meter.exportActive) + "KWH"));
            display.drawString(64,  54, ("Total: " + String(meter.energyTotal) + "KWH"));
            display.display();
          } else { button.screen++; }
          break;

      case 3: // Wifi Info
          display.clear();
          display.setFont(ArialMT_Plain_10);
          display.setTextAlignment(TEXT_ALIGN_LEFT);    
          display.drawString(0, 0,  ("IP: " + WiFi.localIP().toString()));
          display.drawString(0, 12, ("SSID: " + WiFi.SSID() + " (" + String(WifiGetRssiAsQuality((int8_t)WiFi.RSSI())) + "%)"));
          display.drawString(0, 24, ("Frec. Pwm: " + String((float)config.pwmFrequency / 10000) + "Khz"));
          display.drawString(0, 36, ("PWM: " + String(pwm.pwmValue) + "% (" + String(pwm.invert_pwm) + ")"));
          display.drawString(0, 48, printUptimeOled());
          // if (Flags.ntpTime) {
          //   display.drawString(0, 60, printDateOled());
          // }
          display.display();
          break;

      case 4: // Temperaturas
          display.clear();
          display.setFont(ArialMT_Plain_10);
          display.setTextAlignment(TEXT_ALIGN_CENTER);
          display.drawString(64, 0, lang._TEMPERATURES_);
          display.setTextAlignment(TEXT_ALIGN_LEFT);
          display.drawString(0, 12, (lang._INVERTERTEMP_ + String(inverter.temperature) + "ºC"));
          display.drawString(0, 24, (lang._TERMOTEMP_ + String(temperature.temperaturaTermo) + "ºC"));
          display.drawString(0, 36, (lang._TRIACTEMP_ + String(temperature.temperaturaTriac) + "ºC"));
          display.drawString(0, 48, (String(config.nombreSensor) + ": " + String(temperature.temperaturaCustom) + "ºC"));
          display.display();
          break;
      
      case 5: // Build Info
          display.clear();
          display.setFont(ArialMT_Plain_24);
          display.setTextAlignment(TEXT_ALIGN_CENTER);    
          display.drawString(64, 0, "FreeDS");
          display.setFont(ArialMT_Plain_10);
          display.drawString(64, 25, lang._DERIVADOR_);
          display.drawString(64, 40, lang._COMPILATION_);
          display.drawString(64, 50, ("(" + String(compile_date) + ")"));
          display.display();
          break;
    }
  }
#endif
}

void showLogo(String Texto, bool timeDelay)
{
  display.clear();
  //display.flipScreenVertically();
  display.drawFastImage(0, 0, 128, 64, FreeDS);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_10);
  int8_t saltoLinea = Texto.indexOf("\n");
  if (saltoLinea == -1){
    display.drawString(87, 45, Texto);
  } else {
    display.drawString(87, 40, Texto.substring(0, saltoLinea));
    display.drawString(87, 50, Texto.substring(saltoLinea + 1, Texto.length()));
  }
  display.display();
  if (timeDelay) { delay(2000); } // Innecesario salvo para mostrar el mensaje ;-)
}