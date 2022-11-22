/*
  mqtt.ino - FreeDs mqtt functions
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

struct topicData
{
    float *variable;
    char topics[18];
};

topicData topicRegisters[] = {
    &inverter.pw1, "pw1",
    &inverter.pv1v, "pv1v",
    &inverter.pv1c, "pv1c",
    &inverter.pw2, "pw2",
    &inverter.pv2v, "pv2v",
    &inverter.pv2c, "pv2c",
    &inverter.wsolar, "wsolar",
    &inverter.temperature, "invTemp",
    &inverter.wtoday, "wtoday",
    &inverter.wgrid, "wgrid",
    &inverter.wtogrid, "wtogrid",
    &inverter.gridv, "gridv",
    &inverter.currentCalcWatts, "calcWatts",
    &inverter.batteryWatts, "batteryWatts",
    &inverter.batterySoC, "batterySoC",
    &inverter.loadWatts, "loadWatts",
    &inverter.acIn, "AcIn",
    &inverter.acOut, "AcOut",
    &meter.voltage, "voltage",
    &meter.current, "current",
    &temperature.temperaturaTermo, "tempTermo",
    &temperature.temperaturaTriac, "tempTriac",
    &temperature.temperaturaCustom, "tempCustom",
    &config.KwToday, "KwToday",
    &config.KwYesterday, "KwYesterday",
    &config.KwExportToday, "KwExportToday",
    &config.KwExportYesterday, "KwExportYesterday",
    &config.KwTotal, "KwTotal",
    &config.KwExportTotal, "KwExportTotal"
};

void connectToWifi()
{
  if (config.flags.debug2) { INFOV("ConnectToWifi()\n"); }

  Serial.println("Connecting to Wi-Fi...");
  #ifdef OLED
    showLogo(lang._CONNECTING_, false);
  #endif

  wifiMulti.addAP(config.ssid1, config.pass1);
  wifiMulti.addAP(config.ssid2, config.pass2);

  while (wifiMulti.run() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  
  if (config.flags.dhcp == false)
  {
    IPAddress local_IP, gateway, subnet, primaryDNS, secondaryDNS;
    local_IP.fromString((String)config.ip);
    gateway.fromString((String)config.gw);
    subnet.fromString((String)config.mask);
    primaryDNS.fromString((String)config.dns1);
    secondaryDNS.fromString((String)config.dns2);

    if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS))
    {
      Serial.println("Fixed IP -  Failed to configure");
    }
  }
  
  WiFi.setSleep(false);
  WiFi.persistent(false);
  scanDoneCounter = 0;
}

void errorConnectToWifi(void)
{
  Error.ConexionWifi = true;
#ifdef OLED
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 0, "WIFI");
  display.drawString(64, 16, "ERROR");
  display.drawString(64, 30, lang._PRGRESTORE_);
  display.drawString(64, 40, lang._WAIT_);
  display.display();
#endif
  timers.ErrorConexionWifi = millis();
  INFOV(PSTR("AP Not valid, Waiting 30 seconds before restart, press 'prg' button to defaults settings or 'rst' button to restart now\n"));
  
  while ((millis() - timers.ErrorConexionWifi) < 30000)
  {
    if (digitalRead(0) == LOW)
    {
      button.ButtonLongPress = true;
      break;
    }
    yield();
  }
  
  if (button.ButtonLongPress)
  {
    INFOV(PSTR("Prg pressed\n"));
    defaultValues();
    saveEEPROM();
    delay(1000);
  }
  else { INFOV(PSTR("No PRG pressed\n")); }
  ESP.restart();
}

void connectToMqtt()
{
  if (config.flags.debug2) { INFOV("ConnectToMqtt()\n"); }

  if (strcmp("5.8.8.8", config.sensor_ip) != 0 && config.flags.mqtt && !mqttClient.connected())
  {
    INFOV(PSTR("Connecting to MQTT...\n"));
    mqttClient.connect();
    Tickers.disable(2);
  }
}

void WiFiEvent(WiFiEvent_t event)
{
  INFOV(PSTR("[WiFi-event] event: %i\n"), event);

  switch (event)
  {
  case SYSTEM_EVENT_STA_GOT_IP:
    Error.ConexionWifi = false;
    Flags.pwmIsWorking = true;
    INFOV(PSTR("WiFi connected to IP address: %s\n"),WiFi.localIP().toString().c_str());
    delay(1000);

    Tickers.enableAll();
    Tickers.disable(3); // Wifi

    if (!config.flags.mqtt || config.wversion == SOLAX_V2_LOCAL) {
      Serial.printf("Desactivando timer mqtt\n");
      Tickers.disable(2);
      Tickers.disable(5);
    }
    break;

  case SYSTEM_EVENT_STA_DISCONNECTED:
    Error.ConexionWifi = true;
    mqttClient.disconnect();
    INFOV(PSTR("WiFi lost connection\n"));
    receivingData = false;
    processData = false;
    Tickers.disableAll();
    Tickers.enable(0); // Display
    Tickers.enable(3); // Wifi
    if (pwm.invert_pwm > 0 && !config.flags.pwmMan) { Flags.pwmIsWorking = false; shutdownPwm(true, "PWM Down: STA DISCONNECTED\n"); } // PWM Shutdown
    break;

  case SYSTEM_EVENT_WIFI_READY:
  case SYSTEM_EVENT_SCAN_DONE:
    scanDoneCounter++;
    if (scanDoneCounter >= 5) { errorConnectToWifi(); }
    break;

  case SYSTEM_EVENT_STA_START:
  case SYSTEM_EVENT_STA_STOP:
  case SYSTEM_EVENT_STA_CONNECTED:
  case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
  case SYSTEM_EVENT_STA_LOST_IP:
  case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
  case SYSTEM_EVENT_STA_WPS_ER_FAILED:
  case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
  case SYSTEM_EVENT_STA_WPS_ER_PIN:
  case SYSTEM_EVENT_STA_WPS_ER_PBC_OVERLAP:
  case SYSTEM_EVENT_AP_START:
  case SYSTEM_EVENT_AP_STOP:
  case SYSTEM_EVENT_AP_STACONNECTED:
  case SYSTEM_EVENT_AP_STADISCONNECTED:
  case SYSTEM_EVENT_AP_STAIPASSIGNED:
  case SYSTEM_EVENT_AP_PROBEREQRECVED:
  case SYSTEM_EVENT_GOT_IP6:
  case SYSTEM_EVENT_ETH_START:
  case SYSTEM_EVENT_ETH_STOP:
  case SYSTEM_EVENT_ETH_CONNECTED:
  case SYSTEM_EVENT_ETH_DISCONNECTED:
  case SYSTEM_EVENT_ETH_GOT_IP:
  case SYSTEM_EVENT_MAX:
    break;
  }
}

void onMqttConnect(bool sessionPresent)
{
  INFOV("Connected to MQTT\n");
  Tickers.disable(2);
  Tickers.enable(5);
  Error.ConexionMqtt = false;

  static char tmpTopic[33];
  static char topics[][12] = {"pwm","pwmman","pwmmanvalue","screen","pwmfrec","brightness","pwmvalue"};

  int nTopics = sizeof(topics) / sizeof(topics[0]);
  for (int i = 0; i < nTopics; i++)
  {
    sprintf(tmpTopic, "%s/cmnd/%s", config.hostServer, topics[i]);
    mqttClient.subscribe(tmpTopic, 0);
    INFOV("Suscribing to topic %s\n",tmpTopic);
  }

  for (int i = 1; i <= 4; i++)
  {
    sprintf(tmpTopic, "%s/relay/%d/CMND", config.hostServer, i);
    mqttClient.subscribe(tmpTopic, 0);
    INFOV("Suscribing to topic %s\n",tmpTopic);
  }

  if (config.wversion >= MQTT_MODE && config.wversion <= (MQTT_MODE + MODE_STEP - 1)) { suscribeMqttMeter(); }

  if (config.flags.domoticz) { mqttClient.subscribe("domoticz/out", 0); INFOV("Suscribing to topic domoticz/out\n");}
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
  INFOV("Disconnected from MQTT\n");

  Error.ConexionMqtt = true;

  if (WiFi.isConnected() && config.flags.mqtt && strcmp("5.8.8.8", config.sensor_ip) != 0 && !Flags.Updating) { Tickers.enable(2); }
}

void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
  Error.ConexionMqtt = false;
  
  if (config.flags.debug1) { 
    INFOV("Publish received, Topic: %s, Size: %d\n", topic, total);
  }

  if (config.flags.mqtt && strcmp("5.8.8.8", config.sensor_ip) != 0)
  {
    
    if (strcmp(topic, "domoticz/out") == 0 && config.flags.domoticz)
    {
      DeserializationError error = deserializeJson(root, payload, len);

      if (error)
      {
        INFOV("deserializeJson() Domoticz failed: %s\n", error.c_str());
      }
      else
      {
        // PWM
        if ((int)root["idx"] == config.domoticzIdx[0])
        {
          if (config.flags.pwmEnabled != (int)root["nvalue"]) {
            config.flags.pwmEnabled = (int)root["nvalue"];
            if (!config.flags.pwmEnabled) { shutdownPwm(); }
            saveEEPROM();
          }
        }

        // Auto/Manual
        if ((int)root["idx"] == config.domoticzIdx[1])
        {
          boolean status = (int)root["nvalue"] > 0 ? true : false;
          if (config.flags.pwmMan != status) {
            config.flags.pwmMan = status;
            config.flags.pwmMan ? changeToManual() : changeToAuto();
            saveEEPROM();
          }
          
          if ((int)root["svalue1"] != config.manualControlPWM) {
            config.manualControlPWM = (int)root["svalue1"];
            saveEEPROM();
          }
        }

        // Oled
        if ((int)root["idx"] == config.domoticzIdx[2])
        {
          boolean status = (int)root["nvalue"] > 0 ? true : false;
          if (status != config.flags.oledPower) {
            config.flags.oledPower = status;
            timers.OledAutoOff = millis();
            turnOffOled();
            saveEEPROM();
          }
          
          if ((int)root["svalue1"] != config.oledBrightness) {
            config.oledBrightness = (int)root["svalue1"];
            Flags.setBrightness = true;
            saveEEPROM();
          }
        }

        char tmp[50];
        sprintf(tmp, "{\"idx\":%d, \"nvalue\":%d, \"svalue\":\"%d\"}", (int)root["idx"], (int)root["nvalue"], (int)root["svalue1"]);
        publisher("domoticz/in", tmp);  
      }
      return;
    }
    
    if (config.wversion == MQTT_BROKER)
    {
      if (strcmp(topic, config.Meter_mqtt) == 0)
      {
        DeserializationError error = deserializeJson(root, payload, len);

        if (error)
        {
          INFOV("deserializeJson() Meter MQTT failed: %s\n", error.c_str());
        }
        else
        {
          timers.ErrorRecepcionDatos = millis();
          Error.RecepcionDatos = false;
          inverter.wgrid = (float)root["ENERGY"]["Power"]; // Potencia de red (Negativo: de red - Positivo: a red) para usar con los datos de Tasmota
          inverter.gridv = (float)root["ENERGY"]["Voltage"]; // Tension de red
          
          if (!config.flags.changeGridSign) { inverter.wgrid *= -1.0; }
        }
        return;
      }

      if (strcmp(topic, config.Solax_mqtt) == 0)
      {
        DeserializationError error = deserializeJson(root, payload, len);

        if (error)
        {
          INFOV("deserializeJson() Solax MQTT failed: %s\n", error.c_str());
        }
        else
        {
          inverter.pv1c = (float)root["ENERGY"]["Pv1Current"]; // Corriente string 1
          inverter.pv2c = (float)root["ENERGY"]["Pv2Current"]; // Corriente string 2
          inverter.pv1v = (float)root["ENERGY"]["Pv1Voltage"]; // Tension string 1
          inverter.pv2v = (float)root["ENERGY"]["Pv2Voltage"]; // Tension string 2
          inverter.pw1 = (float)root["ENERGY"]["Pv1Power"];    // Potencia string 1
          inverter.pw2 = (float)root["ENERGY"]["Pv2Power"];    // Potencia string 2
          inverter.wtoday = (float)root["ENERGY"]["Today"];    // Potencia solar diaria
          inverter.wsolar = (float)root["ENERGY"]["Power"];    // Potencia solar actual
          inverter.temperature = (float)root["ENERGY"]["Temperature"];    // Temperatura Inversor
        }
        return;
      }
    }

    if (config.wversion == ICC_SOLAR)
    {
      timers.ErrorRecepcionDatos = millis();
      Error.RecepcionDatos = false;

      if (strcmp(topic, "Inverter/GridWatts") == 0) {
        // timers.ErrorVariacionDatos = millis();
        // Error.VariacionDatos = false;
        inverter.wgrid = atof(payload);
        if (!config.flags.changeGridSign) { inverter.wgrid *= -1.0; }
        return;
      }

      if (strcmp(topic, "Inverter/MPPT1_Watts") == 0) { inverter.pw1 = atof(payload); return; }
      if (strcmp(topic, "Inverter/MPPT2_Watts") == 0) { inverter.pw2 = atof(payload); return; }
      if (strcmp(topic, "Inverter/MPPT1_Volts") == 0) { inverter.pv1v = atof(payload); return; }
      if (strcmp(topic, "Inverter/MPPT2_Volts") == 0) { inverter.pv2v = atof(payload); return; }
      if (strcmp(topic, "Inverter/MPPT1_Amps") == 0) { inverter.pv1c = atof(payload); return; }
      if (strcmp(topic, "Inverter/MPPT2_Amps") == 0) { inverter.pv2c = atof(payload); return; }
      if (strcmp(topic, "Inverter/PvWattsTotal") == 0) { inverter.wsolar = atof(payload); return; }
      if (strcmp(topic, "Inverter/SolarKwUse") == 0) { inverter.wtoday = atof(payload); return; }
      if (strcmp(topic, "Inverter/BatteryVolts") == 0) { meter.voltage = atof(payload); return; }
      if (strcmp(topic, "Inverter/BatteryAmps") == 0) { meter.current = atof(payload); return; }
      if (strcmp(topic, "Inverter/BatteryWatts") == 0) { inverter.batteryWatts = atof(payload); return; }
      if (strcmp(topic, config.SoC_mqtt) == 0) { inverter.batterySoC = atof(payload); return; }
      if (strcmp(topic, "Inverter/LoadWatts") == 0) { inverter.loadWatts = atof(payload); return; }
      if (strcmp(topic, "Inverter/Temperature") == 0) { inverter.temperature = atof(payload); return; }
    }

    static char tmpTopic[50]; 

    for (uint8_t i = 1; i <= 4; i++)
    {
      sprintf(tmpTopic, "%s/relay/%d/CMND", config.hostServer, i);
      if (strcmp(topic, tmpTopic) == 0)
      { // pwm control ON-OFF
        INFOV("Mqtt - Relay %d control: %s\n", i, (char)payload[0] == '1' ? "ON" : "OFF");
        uint32_t op;
        if ((char)payload[0] == '1') {
          op = 0x200 << i;
          Flags.data = Flags.data | op;
        } else { 
          op = 0x200 << i;
          Flags.data = Flags.data ^ op;
        }
        relayManualControl(false);
        return;
      }
    }
    
    sprintf(tmpTopic, "%s/cmnd/pwm", config.hostServer);
    if (strcmp(topic, tmpTopic) == 0)
    { // pwm control ON-OFF
      INFOV("Mqtt - PWM control: %s\n", (char)payload[0] == '1' ? "ON" : "OFF");
      config.flags.pwmEnabled = (char)payload[0] == '1' ? true : false;
      if (!config.flags.pwmEnabled) { shutdownPwm(false, "PWM Dowm: Mqtt command received\n"); }
      Flags.pwmIsWorking = true;
      saveEEPROM();
      return;
    }

    sprintf(tmpTopic, "%s/cmnd/pwmman", config.hostServer);
    if (strcmp(topic, tmpTopic) == 0)
    { // pwm control manual ON-OFF
      INFOV("Mqtt - PWM mode set to: %s\n", (char)payload[0] == '1' ? "MANUAL" : "AUTO");
      config.flags.pwmMan = (char)payload[0] == '1' ? true : false;
      config.flags.pwmMan ? changeToManual() : changeToAuto();
      Flags.pwmIsWorking = true;
      saveEEPROM();
      return;
    }

    sprintf(tmpTopic, "%s/cmnd/pwmmanvalue", config.hostServer);
    if (strcmp(topic, tmpTopic) == 0)
    { // Manual pwm control value
      int strData = atoi(payload);
      INFOV("Mqtt - Manual PWM value set to: %i\n", strData);
      config.manualControlPWM = constrain(strData, 0, 100);
      saveEEPROM();
      return;
    }

    sprintf(tmpTopic, "%s/cmnd/pwmfrec", config.hostServer);
    if (strcmp(topic, tmpTopic) == 0)
    { // Pwm frequency
      int strData = atoi(payload);
      INFOV("Mqtt - PWM Frequency set to: %i\n", strData);
      config.pwmFrequency = constrain(strData, 10, 30000);
      saveEEPROM();
      return;
    }

    sprintf(tmpTopic, "%s/cmnd/screen", config.hostServer);
    if (strcmp(topic, tmpTopic) == 0)
    { // Info Screen
      button.screen = constrain(int(payload[0] - '0'), 0, MAX_SCREENS);
      INFOV("Mqtt - Change to Screen: %i\n", button.screen);      
      
      if (config.wversion >= (MODBUS_RTU + MODE_STEP) && button.screen == 2) { button.screen = 1; }
      if ((config.wversion >= MODBUS_RTU && config.wversion <= (MODBUS_RTU + MODE_STEP - 1)) && button.screen == 1) { button.screen = 2; }
      if (!config.flags.sensorTemperatura && button.screen == 5) { button.screen = 6; }
      return;
    }

    sprintf(tmpTopic, "%s/cmnd/brightness", config.hostServer);
    if (strcmp(topic, tmpTopic) == 0)
    { // Oled brightness value
      int strData = atoi(payload);
      INFOV("Mqtt - Change screen brightness to: %i\n", strData);
      config.oledBrightness = ((constrain(strData, 0, 100) * 255) / 100);
      Flags.setBrightness = true;
      return;
    }

    // DEBUG FUNCTION
    sprintf(tmpTopic, "%s/cmnd/pwmvalue", config.hostServer);
    if (strcmp(topic, tmpTopic) == 0)
    { // Manual pwm control value
      uint16_t strData = atoi(payload);
      INFOV("Mqtt - PWM value set to: %i\n", strData);
      pwm.invert_pwm = constrain(strData, 0, 1023);
      config.manualControlPWM = ((pwm.invert_pwm * 100) / 1023);
      return;
    }
  }
}

void suscribeMqttMeter(void)
{
  switch (config.wversion)
  {
    case MQTT_BROKER:
      mqttClient.subscribe(config.Solax_mqtt, 0);
      INFOV("Suscribing to topic %s\n",config.Solax_mqtt);
      mqttClient.subscribe(config.Meter_mqtt, 0);
      INFOV("Suscribing to topic %s\n",config.Meter_mqtt);
      break;
    case ICC_SOLAR:
      mqttClient.subscribe("Inverter/GridWatts", 0);
      mqttClient.subscribe("Inverter/MPPT1_Watts", 0);
      mqttClient.subscribe("Inverter/MPPT2_Watts", 0);
      mqttClient.subscribe("Inverter/MPPT1_Volts", 0);
      mqttClient.subscribe("Inverter/MPPT2_Volts", 0);
      mqttClient.subscribe("Inverter/MPPT1_Amps", 0);
      mqttClient.subscribe("Inverter/MPPT2_Amps", 0);
      mqttClient.subscribe("Inverter/PvWattsTotal", 0);
      mqttClient.subscribe("Inverter/SolarKwUse", 0);
      mqttClient.subscribe("Inverter/BatteryVolts", 0);
      mqttClient.subscribe("Inverter/BatteryAmps", 0);
      mqttClient.subscribe("Inverter/BatteryWatts", 0);
      mqttClient.subscribe("Inverter/LoadWatts", 0);
      mqttClient.subscribe("Inverter/Temperature", 0);
      INFOV("Suscribing to Icc_Solar topics\n");
      mqttClient.subscribe(config.SoC_mqtt, 0);
      INFOV("Suscribing to topic %s\n",config.SoC_mqtt);
      break;
  }
}

void unSuscribeMqtt(void)
{  
  mqttClient.unsubscribe(config.Solax_mqtt);
  mqttClient.unsubscribe(config.Meter_mqtt);
  mqttClient.unsubscribe("Inverter/GridWatts");
  mqttClient.unsubscribe("Inverter/MPPT1_Watts");
  mqttClient.unsubscribe("Inverter/MPPT2_Watts");
  mqttClient.unsubscribe("Inverter/MPPT1_Volts");
  mqttClient.unsubscribe("Inverter/MPPT2_Volts");
  mqttClient.unsubscribe("Inverter/MPPT1_Amps");
  mqttClient.unsubscribe("Inverter/MPPT2_Amps");
  mqttClient.unsubscribe("Inverter/PvWattsTotal");
  mqttClient.unsubscribe("Inverter/SolarKwUse");
  mqttClient.unsubscribe("Inverter/BatteryVolts");
  mqttClient.unsubscribe("Inverter/BatteryAmps");
  mqttClient.unsubscribe("Inverter/BatteryWatts");
  mqttClient.unsubscribe("Inverter/LoadWatts");
  mqttClient.unsubscribe("Inverter/Temperature");
  mqttClient.unsubscribe(config.SoC_mqtt);
  if (!config.flags.domoticz) { mqttClient.unsubscribe("domoticz/out"); }
}

void publisher(const char *topic, const char *topublish)
{
  mqttClient.publish(topic, 0, true, topublish);
}

void publishMqtt()
{
  //  MQTT TOPICS:
  // nombrehost/pwm -> PWM Value
  // nombrehost/pv1c -> corriente string 1
  // nombrehost/pv2c -> corriente string 2
  // nombrehost/pv1v -> tension string 1
  // nombrehost/pv2v -> tension string 2
  // nombrehost/pw1 -> potencia string 1
  // nombrehost/pw2 -> potencia string 2
  // nombrehost/gridv ->  tension de red
  // nombrehost/wsolar ->  Potencia solar
  // nombrehost/temperature ->  Potencia solar
  // nombrehost/wtoday ->  Potencia solar diaria
  // nombrehost/wgrid ->  Potencia de red (Negativo: de red - Positivo: a red)
  // nombrehost/wtogrid -> Potencia enviada a red
  // nombrehost/stat/pwm -> Modo actual de funcionamiento
  // nombrehost/Meter -> En caso de usar el modo meter envía los datos del meter en este Topic

  if (config.flags.debug2) { INFOV("PUBLISHMQTT()\n"); }

  if (config.flags.mqtt && !Error.ConexionMqtt && strcmp("5.8.8.8", config.sensor_ip) != 0)
  {
    static char tmpString[33];
    static char tmpTopic[33];

    int nTopics = sizeof(topicRegisters) / sizeof(topicRegisters[0]);
    for (int i=0; i < nTopics; i++)
    {
      dtostrfd(*topicRegisters[i].variable, 2, tmpString);
      sprintf(tmpTopic, "%s/%s", config.hostServer, topicRegisters[i].topics);
      publisher(tmpTopic, tmpString);
    }

    sprintf(tmpTopic, "%s/pwm", config.hostServer);
    sprintf(tmpString, "%d", pwm.pwmValue);
    publisher(tmpTopic, tmpString);

    config.flags.pwmEnabled ? (config.flags.pwmMan ? strcpy(tmpString, "MAN") : strcpy(tmpString, "AUTO")) : strcpy(tmpString, "OFF");
    
    sprintf(tmpTopic, "%s/stat/pwm", config.hostServer);
    publisher(tmpTopic, tmpString);
    publisher(config.R01_mqtt, digitalRead(PIN_RL1) ? "ON" : "OFF");
    publisher(config.R02_mqtt, digitalRead(PIN_RL2) ? "ON" : "OFF");
    publisher(config.R03_mqtt, digitalRead(PIN_RL3) ? "ON" : "OFF");
    publisher(config.R04_mqtt, digitalRead(PIN_RL4) ? "ON" : "OFF");

    // Domoticz Index
    // PWM ON/OFF
    if (config.flags.domoticz && config.domoticzIdx[0] > 0)  {
      sprintf(tmpTopic, "{\"idx\":%d, \"nvalue\":%d}", config.domoticzIdx[0], (int)config.flags.pwmEnabled);
      publisher("domoticz/in", tmpTopic);
    }
    
    // Auto/Manual
    if (config.flags.domoticz && config.domoticzIdx[1] > 0)  {
      sprintf(tmpTopic, "{\"idx\":%d, \"nvalue\":%d, \"svalue\":\"%d\"}", config.domoticzIdx[1], (int)config.flags.pwmMan, config.manualControlPWM);
      publisher("domoticz/in", tmpTopic);
    }

    // Oled
    if (config.flags.domoticz && config.domoticzIdx[2] > 0)  {
      sprintf(tmpTopic, "{\"idx\":%d, \"nvalue\":%d, \"svalue\":\"%d\"}", config.domoticzIdx[2], (int)config.flags.oledPower, config.oledBrightness);
      publisher("domoticz/in", tmpTopic);
    }

    if (config.wversion >= MODBUS_RTU && config.wversion <= (MODBUS_RTU + MODE_STEP - 1))
    {

      DynamicJsonDocument jsonValues(512);
      static char buffer[512];

      jsonValues["Power"] = meter.activePower;
      jsonValues["Voltage"] = meter.voltage;
      jsonValues["Current"] = meter.current;
      jsonValues["Frequency"] = meter.frequency;
      jsonValues["Factor"] = meter.powerFactor;
      jsonValues["EnergyTotal"] = meter.energyTotal;
      jsonValues["AparentPower"] = meter.aparentPower;
      jsonValues["ReactivePower"] = meter.reactivePower;
      jsonValues["ImportActive"] = meter.importActive;
      jsonValues["ExportActive"] = meter.exportActive;
      jsonValues["ImportReactive"] = meter.importReactive;
      jsonValues["ExportReactive"] = meter.exportReactive;
      jsonValues["PhaseAngle"] = meter.phaseAngle;
      size_t n = serializeJson(jsonValues, buffer);

      sprintf(tmpTopic, "%s/Meter", config.hostServer);
      mqttClient.publish(tmpTopic, 0, true, buffer, n);
    }
  }
}