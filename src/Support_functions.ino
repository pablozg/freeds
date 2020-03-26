/*
  Support_functions.ino - FreeDs support functions
  Derivador de excedentes para ESP32 DEV Kit // Wifi Kit 32

  Based in opends+ (https://github.com/iqas/derivador)
  
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

void getSensorData(void)
{
  if (config.wifi)
  {
    switch (config.wversion)
    {
      case 0: // Solax v2 local mode
        v0_com(); 
        break;
      case 1: // Solax v1
        v1_com(); 
        break;
      case 2: // Solax v2
        m1_com(); 
        break;
      case 4: // DDS2382
      case 5: // DDSU666
      case 6: // SDM120/220
        readMeter();
        break;
      case 9: // Wibee
        getWibeeeData(); 
        break;
      case 10: // Shelly EM
        getShellyData(); 
        break;
      case 11: // Fronius
        fronius_com(); 
        break;
      case 12: // Master FreeDS
        getMasterFreeDsData();
        break;
    }
  }
}

String midString(String str, String start, String finish){
  int locStart = str.indexOf(start);
  if (locStart == -1) return "";
  locStart += start.length();
  int locFinish = str.indexOf(finish, locStart);
  if (locFinish == -1) return "";
  return str.substring(locStart, locFinish);
}

char *dtostrfd(double number, unsigned char prec, char *s)
{
  if ((isnan(number)) || (isinf(number)))
  { // Fix for JSON output (https://stackoverflow.com/questions/1423081/json-left-out-infinity-and-nan-json-status-in-ecmascript)
    strcpy(s, "null");
    return s;
  }
  else
  {
    return dtostrf(number, 1, prec, s);
  }
}

void buildWifiArray(void)
{
  WiFi.scanNetworks();
  for (int i = 0; i < 15; ++i) {
    if(WiFi.SSID(i) == "") { break; }
    scanNetworks[i] = WiFi.SSID(i);
    INFOLN(scanNetworks[i]);
  }
}

void changeScreen(void)
{
  if (digitalRead(0) == LOW)
  {

    if (ButtonState == false && (millis() - lastDebounceTime) > debounceDelay)
    {
      ButtonState = true;
      lastDebounceTime = millis();
    }

    if (((millis() - lastDebounceTime) > 2000) && ButtonLongPress == false)
    {
      ButtonLongPress = true;
      workingMode++;
      if (workingMode > 2)
      {
        workingMode = 0;
      }
      switch (workingMode)
      {
      case 0: // AUTO
        config.P01_on = true;
        config.pwm_man = false;
        break;

      case 1: // MANUAL
        config.P01_on = true;
        config.pwm_man = true;
        break;

      case 2: // OFF
        config.P01_on = false;
        config.pwm_man = false;
        break;
      }
      saveEEPROM();
    }

    if ((millis() - lastDebounceTime) > 10000)
    {
      defaultValues();
      restartFunction();
    }
  }
  else
  {
    if (ButtonState == true)
    {
      if (ButtonLongPress == true)
      {
        ButtonLongPress = false;
        ButtonState = false;
      }
      else
      {
        ButtonState = false;
        timers.OledAutoOff = millis();
        if (config.oledAutoOff && !config.oledPower)
        {
          config.oledPower = true;
          turnOffOled();
        }
        else
        {
          screen++;
          if (screen > MAX_SCREENS)
          {
            screen = 0;
          }
        }
      }
    }
  }
}

void turnOffOled(void)
{
  display.clear();
  config.oledPower ? display.displayOn() : display.displayOff();
}

void restartFunction(void)
{
  
  if (!Flags.firstInit)
  {
    down_pwm(false);
  }

  saveEEPROM();

  INFOLN("RESTARTING IN 3 SEC !!!!");

  uint8_t tcont = 4;
  while (tcont-- > 0)
  {
    INFO(".." + (String)tcont);
    delay(1000);
  }
  ESP.restart();
}

void saveEEPROM(void)
{
  EEPROM.put(0, config);
  EEPROM.commit();
  INFOLN("DATA SAVED!!!!");
}

void remote_api()
{
  DEBUGLN("\r\nremote_api()");
  HTTPClient clientHttp;
  WiFiClient clientWifi;
  clientHttp.setConnectTimeout(4000);

  if ((String)config.remote_api != "" && config.wifi)
  {
    String url = "http://" + (String)config.remote_api;

    url.replace("%pv1c%", String(inverter.pv1c));
    url.replace("%pv2c%", String(inverter.pv2c));
    url.replace("%pv1v%", String(inverter.pv1v));
    url.replace("%pv2v%", String(inverter.pv2v));
    url.replace("%gridv%", String(inverter.gridv));
    url.replace("%wsolar%", String(inverter.wsolar));
    url.replace("%wtoday%", String(inverter.wtoday));
    url.replace("%wgrid%", String(inverter.wgrid));
    url.replace("%pw1%", String(inverter.pw1));
    url.replace("%pw2%", String(inverter.pw2));
    url.replace("%wtogrid%", String(inverter.wtogrid));

    DEBUGLN("REMOTE API REQUEST: " + url);

    clientHttp.begin(clientWifi, url);
    httpcode = clientHttp.GET();

    DEBUGLN("HTTPCODE ERROR: " + (String)httpcode);

    if (httpcode < 0 || httpcode == 404)
      numeroErrorConexionRemoteApi++;
    
    if (httpcode == HTTP_CODE_OK)
    {
      numeroErrorConexionRemoteApi = 0;
      Error.RemoteApi = false;
    }
    clientHttp.end();
    clientWifi.stop();
  }
}

///////////////////////// TIME FUNCTIONS from https://hackaday.io/project/7008-fly-wars-a-hackers-solution-to-world-hunger/log/25043-updated-uptime-counter /////////////////

//************************ Uptime Code - Makes a count of the total up time since last start ****************//

void calc_uptime()
{
  //** Making Note of an expected rollover *****//
  if (millis() >= 3000000000)
  {
    uptime.HighMillis = 1;
  }
  //** Making note of actual rollover **//
  if (millis() <= 100000 && uptime.HighMillis == 1)
  {
    uptime.Rollover++;
    uptime.HighMillis = 0;
  }

  long secsUp = millis() / 1000;
  uptime.Second = secsUp % 60;
  uptime.Minute = (secsUp / 60) % 60;
  uptime.Hour = (secsUp / (60 * 60)) % 24;
  uptime.Day = (uptime.Rollover * 50) + (secsUp / (60 * 60 * 24)); //First portion takes care of a rollover [around 50 days]
};

//******************* Prints the uptime to serial window **********************//
String print_Uptime()
{
  char tmp[33];
  sprintf(tmp, "Uptime: %li días %02d:%02d:%02d", uptime.Day, uptime.Hour, uptime.Minute, uptime.Second);
  return tmp;
};

String print_Uptime_Short()
{
  char tmp[33];
  sprintf(tmp, "%02d:%02d:%02d", uptime.Hour, uptime.Minute, uptime.Second);
  return tmp;
};

String print_Uptime_Oled()
{
  char tmp[33];
  sprintf(tmp, "UPTIME: %li días %02d:%02d:%02d", uptime.Day, uptime.Hour, uptime.Minute, uptime.Second);
  return tmp;
};

void verbose_print_reset_reason(RESET_REASON reason)
{
  switch (reason)
  {
    case 1:
      INFOLN("Vbat power on reset");
      break;
    case 3:
      INFOLN("Software reset digital core");
      break;
    case 4:
      INFOLN("Legacy watch dog reset digital core");
      break;
    case 5:
      INFOLN("Deep Sleep reset digital core");
      break;
    case 6:
      INFOLN("Reset by SLC module, reset digital core");
      break;
    case 7:
      INFOLN("Timer Group0 Watch dog reset digital core");
      break;
    case 8:
      INFOLN("Timer Group1 Watch dog reset digital core");
      break;
    case 9:
      INFOLN("RTC Watch dog Reset digital core");
      break;
    case 10:
      INFOLN("Instrusion tested to reset CPU");
      break;
    case 11:
      INFOLN("Time Group reset CPU");
      break;
    case 12:
      INFOLN("Software reset CPU");
      break;
    case 13:
      INFOLN("RTC Watch dog Reset CPU");
      break;
    case 14:
      INFOLN("for APP CPU, reseted by PRO CPU");
      break;
    case 15:
      INFOLN("Reset when the vdd voltage is not stable");
      break;
    case 16:
      INFOLN("RTC Watch dog reset digital core and rtc module");
      break;
    default:
      INFOLN("NO_MEAN");
  }
}

/// BASIC LOGGING

void addLog(String data, bool line)
{
  //Serial.println(data);
  if (logcount > 9)
    logcount = 0;
  Logging[logcount].timeStamp = print_Uptime_Short();
  Logging[logcount].Message = data;
  Logging[logcount].Message += "{n}";
  logcount++;
  if (Flags.eventsConnected) sendWeblogStreamTest();
}

void sendWeblogStreamTest(void)
{
  String log;

  for (int counter = 0; counter < logcount; counter++)
  {
    log = Logging[counter].timeStamp;
    log += " - ";
    log += Logging[counter].Message;
    events.send(log.c_str(), "weblog");
  }
  logcount = 0;
}

void checkEEPROM(void){
  
  // Paso de versión 0x0A a 0x0B
  if(config.eeinit == 0x0A)
  {
    config.pwm_man = false;
    config.manualControlPWM = 50;
    config.autoControlPWM = 60;
    config.pwmFrequency = 3000;
    config.getDataTime = 1500;
    strcpy(config.remote_api, "");
    config.eeinit = 0x0B;
  }

  // Paso de versión 0x0B a 0x0C
  if(config.eeinit == 0x0B)
  {
    config.baudiosMeter = 9600;
    config.idMeter = 1; 
    config.eeinit = 0x0C;
  }

  // Paso de versión 0x0C a 0x0D
  if(config.eeinit == 0x0C)
  {
    config.pwmSlaveOn = 0;
    config.potmanpwm = 0;
    config.flags.potManPwmActive = false;
    config.eeinit = 0x0D;
    saveEEPROM();
  }
}
