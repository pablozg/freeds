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
        temporizadorOledAutoOff = millis();
        if (config.oledAutoOff && !config.oledPower) {
          config.oledPower = true;
          turnOffOled();
        } else {
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
  saveEEPROM();
  if (!firstInit)
  {
    down_pwm(false);
  }
  
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


  if ((String)config.remote_api != "" && config.wifi)
  {
    String buf = "http://" + (String)config.remote_api;
    
    buf.replace("%pv1c%", String(inverter.pv1c));
    buf.replace("%pv2c%", String(inverter.pv2c));
    buf.replace("%pv1v%", String(inverter.pv1v));
    buf.replace("%pv2v%", String(inverter.pv2v));
    buf.replace("%gridv%", String(inverter.gridv));
    buf.replace("%wsolar%", String(inverter.wsolar));
    buf.replace("%wtoday%", String(inverter.wtoday));
    buf.replace("%wgrid%", String(inverter.wgrid));
    buf.replace("%pw1%", String(inverter.pw1));
    buf.replace("%pw2%", String(inverter.pw2));
    buf.replace("%wtogrid%", String(inverter.wtogrid));

    DEBUGLN("REMOTE API REQUEST: " + buf);

    char bufferdata[buf.length() + 1];
    buf.toCharArray(bufferdata, (buf.length() + 1));
    http.begin(bufferdata);
    httpcode = http.GET();

    DEBUGLN("HTTPCODE ERROR: " + (String)httpcode);

    if (httpcode < 0 || httpcode == 404)
      numeroErrorConexionRemoteApi++;
    if (httpcode == HTTP_CODE_OK)
    {
      numeroErrorConexionRemoteApi = 0;
      errorRemoteApi = false;
    }
    http.end();
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
  return "Uptime: " + String(uptime.Day) + " Días " + String(uptime.Hour) + " Horas " + String(uptime.Minute) + " Minutos " + String(uptime.Second) + " Segundos";
};