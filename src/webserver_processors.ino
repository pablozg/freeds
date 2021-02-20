/*
  webserver_processors.ino - FreeDs webserver processor
  Derivador de excedentes para ESP32 DEV Kit // Wifi Kit 32
  
  Copyright (C) 2020 Theo arendst (https://github.com/arendst/Tasmota)
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

String workingModeString(void)
{
  if (config.wversion == SOLAX_V2_LOCAL)
  {
    return "Solax Wifi v2 local";
  }
  if (config.wversion == SOLAX_V1)
  {
    return "Solax Wifi v1 - Hibridos";
  }
  if (config.wversion == SOLAX_V2)
  {
    return "Solax Wifi v2 (ESP01)";
  }
  if (config.wversion == MQTT_BROKER)
  {
    return "MQTT Server (Tasmota Json)";
  }
  if (config.wversion == DDS238_METER)
  {
    return "Meter DDS238-2(4) Modbus";
  }
  if (config.wversion == DDSU666_METER)
  {
    return "Meter DDSU666 Modbus";
  }
  if (config.wversion == SDM_METER)
  {
    return "Meter SDM120 / SDM220 Modbus";
  }
  if (config.wversion == GOODWE)
  {
    return "GoodWe ES/EM";
  }
  if (config.wversion == MUSTSOLAR)
  {
    return "MustSolar Modbus (En desarrollo)";
  }
  if (config.wversion == SMA_BOY)
  {
    return "SMA (Sunny Boy)";
  }
  if (config.wversion == SMA_ISLAND)
  {
    return "SMA (Sunny Island)";
  }
  if (config.wversion == WIBEEE)
  {
    return "Wibeee";
  }
  if (config.wversion == SHELLY_EM)
  {
    return "Shelly EM";
  }
  if (config.wversion == FRONIUS_API)
  {
    return "Fronius (API)";
  }
  if (config.wversion == SLAVE_MODE)
  {
    return "Slave FreeDS";
  }
  if (config.wversion == ICC_SOLAR)
  {
    return "ICC Solar (Mqtt)";
  }
  if (config.wversion == VICTRON)
  {
    return "Victron Modbus TCP";
  }
  if (config.wversion == FRONIUS_MODBUS)
  {
    return "Fronius Modbus TCP";
  }
  if (config.wversion == HUAWEI_MODBUS)
  {
    return "Huawei Modbus TCP";
  }
  if (config.wversion == SOLAREDGE)
  {
    return "SolarEdge Modbus TCP";
  }
  if (config.wversion == WIBEEE_MODBUS)
  {
    return "Wibeee Modbus TCP (En desarrollo)";
  }
  if (config.wversion == SCHNEIDER)
  {
    return "Schneider Modbus TCP (En desarrollo)";
  }
  if (config.wversion == INGETEAM)
  {
    return "Ingeteam Modbus TCP";
  }
  return String();
}

String processorFreeDS(const String &var)
{
  if (var == "SELECT_MODE")
  {
    return "<select class='form-control select2 mg-b-2 mg-r-30' onchange='run()' id='version'>"  
                                                                              "<option value='" + String(SOLAX_V1) + "'" +
           String((config.wversion == SOLAX_V1) ? " selected='selected' " : " ") + ">Solax Wifi v1 - Hibridos</option>"
                                                                              "<option value='" + String(SOLAX_V2) + "'" +
           String((config.wversion == SOLAX_V2) ? " selected='selected' " : " ") + ">Solax Wifi v2 (ESP01)</option>"
                                                                              "<option value='" + String(SOLAX_V2_LOCAL) + "'" +
           String((config.wversion == SOLAX_V2_LOCAL) ? " selected='selected' " : " ") + ">Solax Wifi v2 local</option>"
                                                                              "<option value='" + String(FRONIUS_API) + "'" +
           String((config.wversion == FRONIUS_API) ? " selected='selected' " : " ") + ">Fronius (API)</option>" +
                                                                              "<option value='" + String(MQTT_BROKER) + "'" +
           String((config.wversion == MQTT_BROKER) ? " selected='selected' " : " ") + ">MQTT Server (Tasmota Json)</option>"
                                                                              "<option value='" + String(DDS238_METER) + "'" +
           String((config.wversion == DDS238_METER) ? " selected='selected' " : " ") + ">Meter DDS238-2(4) Modbus</option>"
                                                                              "<option value='" + String(DDSU666_METER) + "'" +
           String((config.wversion == DDSU666_METER) ? " selected='selected' " : " ") + ">Meter DDSU666 Modbus</option>"
                                                                              "<option value='" + String(SDM_METER) + "'" +
           String((config.wversion == SDM_METER) ? " selected='selected' " : " ") + ">Meter SDM120 / SDM220 Modbus</option>"
                                                                              "<option value='"+ String(GOODWE) + "'" +
           String((config.wversion == GOODWE) ? " selected='selected' " : " ") + ">Goodwe ES/EM</option>"
                                                                              "<option value='"+ String(MUSTSOLAR) + "'" +
           String((config.wversion == MUSTSOLAR) ? " selected='selected' " : " ") + ">MustSolar Modbus (En desarrollo)</option>"
                                                                              "<option value='" + String(SMA_BOY) + "'" +
           String((config.wversion == SMA_BOY) ? " selected='selected' " : " ") + ">SMA (Sunny Boy)</option>"
                                                                              "<option value='" + String(SMA_ISLAND) + "'" +
           String((config.wversion == SMA_ISLAND) ? " selected='selected' " : " ") + ">SMA (Sunny Island)</option>"
                                                                              "<option value='" + String(VICTRON) + "'" +
           String((config.wversion == VICTRON) ? " selected='selected' " : " ") + ">Victron Modbus TCP</option>" +
                                                                              "<option value='" + String(FRONIUS_MODBUS) + "'" +
           String((config.wversion == FRONIUS_MODBUS) ? " selected='selected' " : " ") + ">Fronius Modbus TCP</option>" +
                                                                              "<option value='" + String(HUAWEI_MODBUS) + "'" +
           String((config.wversion == HUAWEI_MODBUS) ? " selected='selected' " : " ") + ">Huawei Modbus TCP</option>" +
                                                                              "<option value='" + String(SOLAREDGE) + "'" +
           String((config.wversion == SOLAREDGE) ? " selected='selected' " : " ") + ">SolarEdge Modbus TCP</option>" +
                                                                              "<option value='" + String(SCHNEIDER) + "'" +
           String((config.wversion == SCHNEIDER) ? " selected='selected' " : " ") + ">Schneider Modbus TCP (En desarrollo)</option>" +
                                                                              "<option value='" + String(INGETEAM) + "'" +
           String((config.wversion == INGETEAM) ? " selected='selected' " : " ") + ">Ingeteam Modbus TCP</option>" +
                                                                              "<option value='" + String(WIBEEE_MODBUS) + "'" +
           String((config.wversion == WIBEEE_MODBUS) ? " selected='selected' " : " ") + ">Wibeee Modbus TCP (En desarrollo)</option>" +
                                                                              "<option value='" + String(WIBEEE) + "'" +
           String((config.wversion == WIBEEE) ? " selected='selected' " : " ") + ">Wibeee</option>" +
                                                                              "<option value='" + String(SHELLY_EM) + "'" +                                                                   
           String((config.wversion == SHELLY_EM) ? " selected='selected' " : " ") + ">Shelly EM</option>"
                                                                              "<option value='" + String(ICC_SOLAR) + "'" +                                                                   
           String((config.wversion == ICC_SOLAR) ? " selected='selected' " : " ") + ">ICC Solar (Mqtt)</option>"
                                                                              "<option value='" + String(SLAVE_MODE) + "'" +
           String((config.wversion == SLAVE_MODE) ? " selected='selected' " : " ") + ">Slave FreeDS</option>" +
           "</select>";
  }

  if (var == "SELECT_LANGUAGE")
  {
    return "<li class='nav-item'><a class='nav-link'><select class='form-control country'>"
              "<option value='es'>Español</option>"
              "<option value='en'>English</option>"
              "<option value='pt'>Português</option>"
              "<option value='ca'>Català</option>"
              "<option value='ga'>Galego</option>"
            "</select></a></li>";
  }

  if (var == "MESSAGE")
  {
    switch (webMessageResponse)
    {
    case 1:
      webMessageResponse = 0;
      return "<div id='Info' class='alert alert-bordered alert-success' role='alert'>"
             "<button type='button' class='close' data-dismiss='alert' aria-label='Close'><span aria-hidden='true'>&times;</span></button>"
             "<div class='d-flex align-items-center justify-content-start'><i class='icon ion-ios-checkmark alert-icon tx-32 mg-t-5 mg-xs-t-0'></i>"
             "Configuración guardada correctamente.</div></div>";
      break;
    case 2:
      webMessageResponse = 0;
      return "<div id='Info' class='alert alert-bordered alert-success' role='alert'>"
             "<button type='button' class='close' data-dismiss='alert' aria-label='Close'><span aria-hidden='true'>&times;</span></button>"
             "<div class='d-flex align-items-center justify-content-start'><i class='icon ion-ios-checkmark alert-icon tx-32 mg-t-5 mg-xs-t-0'></i>"
             "Reiniciando el dispositivo.</div></div>";
      break;
    }
  }

  if (var == "VERSION_CODE")
  {
    return String(FPSTR(version)) + " " + String(FPSTR(beta));
  }
  if (var == "FECHA_COMPILACION")
  {
    return String(FPSTR(compile_date));
  }

  return String();
}

String processorRed(const String &var)
{
  if (var == "WORKING_MODE")
  {
    return workingModeString();
  }
  if (var == "WIFI1")
  {
    char tmp[80];
    String wifi = "<select id='wifi1' name='wifi1' class='form-control select2'><option disabled selected>Seleccione una red</option>";
                  for (int i = 0; i < 15; ++i) {
                    if (scanNetworks[i] == "") { break; }
                    sprintf(tmp,"%s (%d&#37;, %d dBm)", scanNetworks[i].c_str(), WifiGetRssiAsQuality(rssiNetworks[i]), rssiNetworks[i]);
                    if (scanNetworks[i] == String(config.ssid1)){ wifi +="<option value='" + scanNetworks[i] + "' selected>" + String(tmp) + "</option>"; }
                    else { wifi +="<option value='" + scanNetworks[i] + "'>" + String(tmp) + "</option>"; }
                  }
           wifi += "</select>";
    return wifi;
  }
  if (var == "PASS1") { return String(config.pass1); }
  if (var == "PASS2") { return String(config.pass2); }
  if (var == "WIFI2")
  {
    char tmp[80];
    String wifi = "<select id='wifi2' name='wifi2' class='form-control select2'><option disabled selected>Seleccione una red</option>";
                  for (int i = 0; i < 15; ++i) {
                    if (scanNetworks[i] == "") { break; }
                    sprintf(tmp,"%s (%d&#37;, %d dBm)", scanNetworks[i].c_str(), WifiGetRssiAsQuality(rssiNetworks[i]), rssiNetworks[i]);
                    if (scanNetworks[i] == String(config.ssid2)){ wifi +="<option value='" + scanNetworks[i] + "' selected>" + String(tmp) + "</option>"; }
                    else { wifi +="<option value='" + scanNetworks[i] + "'>" + String(tmp) + "</option>"; }
                  }
           wifi += "</select>";
    return wifi;
  }

  if (var == "HOST")
  {
    return String(config.hostServer);
  }
  if (var == "IP")
  {
    return config.flags.dhcp ? WiFi.localIP().toString() : String(config.ip);
  }
  if (var == "GW")
  {
    return config.flags.dhcp ? WiFi.gatewayIP().toString() : String(config.gw);
  }
  if (var == "MASK")
  {
    return config.flags.dhcp ? WiFi.subnetMask().toString() : String(config.mask);
  }
  if (var == "DNS1")
  {
    return config.flags.dhcp ? WiFi.dnsIP(0).toString() : String(config.dns1);
  }
  if (var == "DNS2")
  {
    return config.flags.dhcp ? WiFi.dnsIP(1).toString() : String(config.dns2);
  }
  if (var == "VERSION_CODE")
  {
    return String(FPSTR(version));
  }
  if (var == "FECHA_COMPILACION")
  {
    return String(FPSTR(compile_date));
  }
  if (var == "DHCP")
  {
    return config.flags.dhcp ? "checked" : "";
  }
  
  return String();
}

String processorMqtt(const String &var)
{
  if (var == "WORKING_MODE")
  {
    return workingModeString();
  }
  if (var == "MQTT_ACTIVE")
  {
    return config.flags.mqtt ? "checked" : "";
  }
  if (var == "BROKER")
  {
    return String(config.MQTT_broker);
  }
  if (var == "MQTTUSER")
  {
    return String(config.MQTT_user);
  }
  if (var == "MQTTPASS")
  {
    return String(config.MQTT_password);
  }
  if (var == "MQTTPORT")
  {
    return String(config.MQTT_port);
  }
  if (var == "MQTTPUBLISH")
  {
    return String(config.publishMqtt);
  }
  if (var == "MQTTR1")
  {
    return String(config.R01_mqtt);
  }
  if (var == "MQTTR2")
  {
    return String(config.R02_mqtt);
  }
  if (var == "MQTTR3")
  {
    return String(config.R03_mqtt);
  }
  if (var == "MQTTR4")
  {
    return String(config.R04_mqtt);
  }

  if (var == "MQTTMETER")
  {
    if (config.wversion == MQTT_BROKER)
    {
      return "<div class='row mg-t-2'><label class='col-sm-4 form-control-label language' key='SOLAXTOPIC'></label><div class='col-sm-8 mg-t-10 mg-sm-t-0'>"
             "<input id='solax' name='solax' type='text' class='form-control' maxlength='50' value='" +
             String(config.Solax_mqtt) + "'></div></div>"
                                           "<div class='row mg-t-2'><label class='col-sm-4 form-control-label language' key='METERTOPIC'></label><div class='col-sm-8 mg-t-10 mg-sm-t-0'>"
                                           "<input id='meter' name='meter' type='text' class='form-control' maxlength='50' value='" +
             String(config.Meter_mqtt) + "'></div></div>";
    }
    if (config.wversion == ICC_SOLAR)
    {
      return "<div class='row mg-t-2'><label class='col-sm-4 form-control-label language' key='SOCTOPIC'></label><div class='col-sm-8 mg-t-10 mg-sm-t-0'>"
             "<input id='soctopic' name='soctopic' type='text' class='form-control' maxlength='50' value='" +
             String(config.SoC_mqtt) + "'></div></div>";
    }
  }
  if (var == "DOMOTICZ")
  {
    return config.flags.domoticz ? "checked" : "";
  }
  if (var == "IDXPWM")
  {
    return String(config.domoticzIdx[0]);
  }
  if (var == "IDXMAN")
  {
    return String(config.domoticzIdx[1]);
  }
  if (var == "IDXOLED")
  {
    return String(config.domoticzIdx[2]);
  }
  if (var == "VERSION_CODE")
  {
    return String(FPSTR(version));
  }
  if (var == "FECHA_COMPILACION")
  {
    return String(FPSTR(compile_date));
  }
  return String();
}

String processorConfig(const String &var)
{
  if (var == "WORKING_MODE")
  {
    return workingModeString();
  }

  if (var == "STOREDPASS")
  {
    return String(config.password);
  }

  if (var == "OFFGRID")
  {
    return config.flags.offGrid ? "checked" : "";
  }
  if (var == "OFFGRIDMODE")
  {
    if (config.flags.offgridVoltage) {
      return "<div class='row mg-t-2'><label class='col-sm-4 form-control-label language' key='OFFGRIDVOLTAGE'></label><div class='col-sm-8 mg-t-10 mg-sm-t-0'>"
             "<input id='soc' name='soc' type='text' class='form-control' maxlength='6' value='" + String(config.batteryVoltage) + "'></div></div>";
    } else {
      return "<div class='row mg-t-2'><label class='col-sm-4 form-control-label language' key='OFFGRIDSOC'></label><div class='col-sm-8 mg-t-10 mg-sm-t-0'>"
             "<input id='soc' name='soc' type='text' class='form-control' maxlength='3' value='" + String(config.soc) + "' onchange='checkSoCValue();'></div></div>";
    }
  }

  if (var == "BATTWATTS")
  {
    return String(config.battWatts);
  }

  if (var == "WIFIS")
  {
    if (config.wversion == SOLAX_V2_LOCAL)
    {
      // return "<label id='labelModo' class='col-sm-4 form-control-label'>IP (Auto):</label>"
      //        "<div id='divModo' class='col-sm-8 mg-t-10 mg-sm-t-0'><input id='wifis' type=\"text\" class=\"form-control select2\" maxlength=\"30\" value=\"" +
      //        String(config.sensor_ip) + "\" name=\"wifis\" disabled /></div>";
      return "<label id='labelModo' class='col-sm-4 form-control-label'>IP Solax (Lan / Local: 5.8.8.8):</label>"
             "<div id='divModo' class='col-sm-8 mg-t-10 mg-sm-t-0'><input id='wifis' type=\"text\" class=\"form-control select2\" maxlength=\"30\" value=\"" +
             String(config.sensor_ip) + "\" name=\"wifis\"/></div>";
    }
    if (config.wversion == SOLAX_V1)
    {
      return "<label id='labelModo' class='col-sm-4 form-control-label'>IP Wifi V1 solax:</label>"
             "<div id='divModo' class='col-sm-8 mg-t-10 mg-sm-t-0'><input id='wifis' type=\"text\" class=\"form-control select2\" maxlength=\"30\" value=\"" +
             String(config.sensor_ip) + "\" name=\"wifis\"/></div>";
    }
    if (config.wversion == SOLAX_V2)
    {
      char tmp[50];
      String wifi = "<label id='labelModo' class='col-sm-4 form-control-label'>SSID Wifi inversor:</label>";
             wifi += "<div id='divModo' class='col-sm-8 mg-t-10 mg-sm-t-0'><select id='wifis' name='wifis' class='form-control select2'>";
             wifi += "<option disabled selected>Seleccione una red</option>";
                      for (int i = 0; i < 15; ++i) {
                        if (scanNetworks[i] == "") { break; }
                        sprintf(tmp,"%s (%d&#37;, %d dBm)", scanNetworks[i].c_str(), WifiGetRssiAsQuality(rssiNetworks[i]), rssiNetworks[i]);
                        if (scanNetworks[i] == String(config.ssid_esp01)){ wifi +="<option value='" + scanNetworks[i] + "' selected>" + String(tmp) + "</option>"; }
                        else { wifi +="<option value='" + scanNetworks[i] + "'>" + String(tmp) + "</option>"; }
                      }
             wifi += "</select></div>";
      return wifi;
    }
    if (config.wversion >= MODBUS_TCP && config.wversion <= (MODBUS_TCP + MODE_STEP - 1))
    {
      return "<label id='labelModo' class='col-sm-4 form-control-label'>IP Modbus TCP:</label>"
             "<div id='divModo' class='col-sm-8 mg-t-10 mg-sm-t-0'><input id='wifis' type=\"text\" class=\"form-control select2\" maxlength=\"30\" value=\"" +
             String(config.sensor_ip) + "\" name=\"wifis\"/></div>";
    }
    if (config.wversion == WIBEEE)
    {
      return "<label id='labelModo' class='col-sm-4 form-control-label'>IP Wibeee:</label>"
             "<div id='divModo' class='col-sm-8 mg-t-10 mg-sm-t-0'><input id='wifis' type=\"text\" class=\"form-control select2\" maxlength=\"30\" value=\"" +
             String(config.sensor_ip) + "\" name=\"wifis\"/></div>";
    }
    if (config.wversion == SHELLY_EM)
    {
      return "<label id='labelModo' class='col-sm-4 form-control-label'>IP Shelly EM:</label>"
             "<div id='divModo' class='col-sm-8 mg-t-10 mg-sm-t-0'><input id='wifis' type=\"text\" class=\"form-control select2\" maxlength=\"30\" value=\"" +
             String(config.sensor_ip) + "\" name=\"wifis\"/></div>";
    }
    if (config.wversion == FRONIUS_API)
    {
      return "<label id='labelModo' class='col-sm-4 form-control-label'>IP Fronius:</label>"
             "<div id='divModo' class='col-sm-8 mg-t-10 mg-sm-t-0'><input id='wifis' type=\"text\" class=\"form-control select2\" maxlength=\"30\" value=\"" +
             String(config.sensor_ip) + "\" name=\"wifis\"/></div>";
    }
    if (config.wversion == SLAVE_MODE)
    {
      return "<label id='labelModo' class='col-sm-4 form-control-label'>IP Master FreeDS:</label>"
             "<div id='divModo' class='col-sm-8 mg-t-10 mg-sm-t-0'><input id='wifis' type=\"text\" class=\"form-control select2\" maxlength=\"30\" value=\"" +
             String(config.sensor_ip) + "\" name=\"wifis\"/></div>";
    }
    if (config.wversion == GOODWE)
    {
      return "<label id='labelModo' class='col-sm-4 form-control-label'>IP GoodWe:</label>"
             "<div id='divModo' class='col-sm-8 mg-t-10 mg-sm-t-0'><input id='wifis' type=\"text\" class=\"form-control select2\" maxlength=\"30\" value=\"" +
             String(config.sensor_ip) + "\" name=\"wifis\"/></div>";
    }
    // MQTT
    if (config.wversion >= MQTT_MODE && config.wversion <= (MQTT_MODE + MODE_STEP - 1))
    {
      return "<label id='labelModo' class='col-sm-4 form-control-label'>MQTT Broker:</label>"
             "<div id='divModo' class='col-sm-8 mg-t-10 mg-sm-t-0'><input id='wifis' type=\"text\" class=\"form-control select2\" maxlength=\"30\" value=\"" +
             String(config.MQTT_broker) + "\" name=\"wifis\" disabled /></div>";
    }
    if (config.wversion == DDS238_METER)
    {
      return "<label id='labelModo' class='col-sm-4 form-control-label'>Modbus:</label>"
             "<div id='divModo' class='col-sm-8 mg-t-10 mg-sm-t-0'><input id='wifis' type=\"text\" class=\"form-control select2\" maxlength=\"30\" value=\"DDS238-2\" name=\"wifis\" disabled /></div>";
    }
    if (config.wversion == DDSU666_METER)
    {
      return "<label id='labelModo' class='col-sm-4 form-control-label'>Modbus:</label>"
             "<div id='divModo' class='col-sm-8 mg-t-10 mg-sm-t-0'><input id='wifis' type=\"text\" class=\"form-control select2\" maxlength=\"30\" value=\"DDSU666\" name=\"wifis\" disabled /></div>";
    }
    if (config.wversion == SDM_METER)
    {
      return "<label id='labelModo' class='col-sm-4 form-control-label'>Modbus:</label>"
             "<div id='divModo' class='col-sm-8 mg-t-10 mg-sm-t-0'><input id='wifis' type=\"text\" class=\"form-control select2\" maxlength=\"30\" value=\"SDM120 / SDM 220\" name=\"wifis\" disabled /></div>";
    }
    if (config.wversion == MUSTSOLAR)
    {
      return "<label id='labelModo' class='col-sm-4 form-control-label'>Modbus:</label>"
             "<div id='divModo' class='col-sm-8 mg-t-10 mg-sm-t-0'><input id='wifis' type=\"text\" class=\"form-control select2\" maxlength=\"30\" value=\"MustSolar\" name=\"wifis\" disabled /></div>";
    }
  }

  if (var == "CHANGEGRIDSIGN")
  {
    return config.flags.changeGridSign ? "checked" : "";
  }

  if (var == "IDMETER")
  {
    return String(config.idMeter);
  }

  if (var == "AUTOPOWEROFF")
  {
    return config.flags.oledAutoOff ? "checked" : "";
  }

  if (var == "AUTOPOWEROFFTIME")
  {
    return String(config.oledControlTime);
  }

  if (var == "SENSORTEMP")
  {
    return config.flags.sensorTemperatura ? "checked" : "";
  }

  if (var == "TEMPOFF")
  {
    return String(config.temperaturaApagado);
  }

  if (var == "TEMPON")
  {
    return String(config.temperaturaEncendido);
  }

  if (var == "SENSORMODOAUTO")
  {
    return (config.modoTemperatura == 1 || config.modoTemperatura == 3) ? "checked" : "";
  }

  if (var == "SENSORMODOMANUAL")
  {
    return (config.modoTemperatura == 2 || config.modoTemperatura == 3) ? "checked" : "";
  }
  if (var == "TERMOADDRS")
  {
    char tmp[33];
    String addrs = "<select id='termoaddrs' name='termoaddrs' class='form-control select2'><option value='0' selected>Seleccione un sensor</option>";
    for (int i = 0; i < 15; ++i) {
      if (temperature.tempSensorAddress[i][0] == 0) {break;}
      sprintf(tmp,"0x%.2X 0x%.2X 0x%.2X 0x%.2X", temperature.tempSensorAddress[i][4], temperature.tempSensorAddress[i][5], temperature.tempSensorAddress[i][6], temperature.tempSensorAddress[i][7]);
      if (memcmp(temperature.tempSensorAddress[i], config.termoSensorAddress, 8) == 0) {
        addrs +="<option value='" + String(i + 1) + "' selected>" + "Sensor Id " + String(i + 1) + " (" + String(tmp) + ")</option>";
      } else {
        addrs +="<option value='" + String(i + 1) + "'>" + "Sensor Id " + String(i + 1) + " (" + String(tmp) + ")</option>";
      }
    }
    addrs += "</select>";
    return addrs;
  }
  if (var == "TRIACADDRS")
  {
    char tmp[33];
    String addrs = "<select id='triacaddrs' name='triacaddrs' class='form-control select2'><option value='0' selected>Seleccione un sensor</option>";
    for (int i = 0; i < 15; ++i) {
      if (temperature.tempSensorAddress[i][0] == 0) {break;}
      sprintf(tmp,"0x%.2X 0x%.2X 0x%.2X 0x%.2X", temperature.tempSensorAddress[i][4], temperature.tempSensorAddress[i][5], temperature.tempSensorAddress[i][6], temperature.tempSensorAddress[i][7]);
      if (memcmp(temperature.tempSensorAddress[i], config.triacSensorAddress, 8) == 0) {
        addrs +="<option value='" + String(i + 1) + "' selected>" + "Sensor Id " + String(i + 1) + " (" + String(tmp) + ")</option>";
      } else {
        addrs +="<option value='" + String(i + 1) + "'>" + "Sensor Id " + String(i + 1) + " (" + String(tmp) + ")</option>";
      }
    }
    addrs += "</select>";
    return addrs;
  }

  if (var == "CUSTOMADDRS")
  {
    char tmp[33];
    String addrs = "<select id='customaddrs' name='customaddrs' class='form-control select2'><option value='0' selected>Seleccione un sensor</option>";
    for (int i = 0; i < 15; ++i) {
      if (temperature.tempSensorAddress[i][0] == 0) {break;}
      sprintf(tmp,"0x%.2X 0x%.2X 0x%.2X 0x%.2X", temperature.tempSensorAddress[i][4], temperature.tempSensorAddress[i][5], temperature.tempSensorAddress[i][6], temperature.tempSensorAddress[i][7]);
      if (memcmp(temperature.tempSensorAddress[i], config.customSensorAddress, 8) == 0) {
        addrs +="<option value='" + String(i + 1) + "' selected>" + "Sensor Id " + String(i + 1) + " (" + String(tmp) + ")</option>";
      } else {
        addrs +="<option value='" + String(i + 1) + "'>" + "Sensor Id " + String(i + 1) + " (" + String(tmp) + ")</option>";
      }
    }
    addrs += "</select>";
    return addrs;
  }
  
  if (var == "CUSTOMSENSOR")
  {
    return String(config.nombreSensor);
  }

  if (var == "MAXERRORTIME")
  {
    return String(config.maxErrorTime);
  }

  if (var == "ALEXA")
  {
    return config.flags.alexaControl ? "checked" : "";
  }

  if (var == "GETDATATIME")
  {
    return String(config.getDataTime);
  }

  if (var == "VERSION_CODE")
  {
    return String(FPSTR(version));
  }

  if (var == "FECHA_COMPILACION")
  {
    return String(FPSTR(compile_date));
  }
  
  return String();
}

String processorSalidas(const String &var)
{
  if (var == "WORKING_MODE")
  {
    return workingModeString();
  }
  if (var == "PWMACTIVE")
  {
    return config.flags.pwmEnabled ? "checked" : "";
  }
  if (var == "WATTSTARIFF")
  {
    return String(config.maxWattsTariff);
  }
  if (var == "LOADWATTS")
  {
    return String(config.attachedLoadWatts);
  }
  if (var == "POTTARGET")
  {
    return String(config.potTarget);
  }
  if (var == "LOWCOSTACTIVE")
  {
    return config.flags.dimmerLowCost ? "checked" : "";
  }
  if (var == "MAXPWMLOWCOST")
  {
    return String(config.maxPwmLowCost);
  }
  if (var == "MANPWM")
  {
    return String(config.manualControlPWM);
  }
  if (var == "SLAVEPWM")
  {
    return String(config.pwmSlaveOn);
  }
  if (var == "POTMANPWM")
  {
    return String(config.potManPwm);
  }
  if (var == "POTPWMACTIVE")
  {
    return config.flags.potManPwmActive ? "checked" : "";
  }
  if (var == "TIMERACTIVE")
  {
    if (Flags.ntpTime)
    {
      return config.flags.timerEnabled ? "checked" : "";  
    } else {
      return "disabled=\"disabled\"";
    }
  }
  if (var == "TIMERSTART")
  {
    char tmp[14];
    sprintf(tmp,"value=\"%02d:%02d\"", getHour(config.timerStart), getMin(config.timerStart));
    return String(tmp);
  }
  if (var == "TIMERSTOP")
  {
    char tmp[14];
    sprintf(tmp,"value=\"%02d:%02d\"", getHour(config.timerStop), getMin(config.timerStop));
    return String(tmp);
  }
  if (var == "AUTOPWM")
  {
    return String(config.autoControlPWM);
  }
  if (var == "R01MIN")
  {
    return String(config.R01Min);
  }
  if (var == "R01POTON")
  {
    return String(config.R01PotOn);
  }
  if (var == "R01POTOFF")
  {
    return String(config.R01PotOff);
  }
  if (var == "R02MIN")
  {
    return String(config.R02Min);
  }
  if (var == "R02POTON")
  {
    return String(config.R02PotOn);
  }
  if (var == "R02POTOFF")
  {
    return String(config.R02PotOff);
  }
  if (var == "R03MIN")
  {
    return String(config.R03Min);
  }
  if (var == "R03POTON")
  {
    return String(config.R03PotOn);
  }
  if (var == "R03POTOFF")
  {
    return String(config.R03PotOff);
  }
  if (var == "R04MIN")
  {
    return String(config.R04Min);
  }
  if (var == "R04POTON")
  {
    return String(config.R04PotOn);
  }
  if (var == "R04POTOFF")
  {
    return String(config.R04PotOff);
  }

  if (var == "R01MAN")
  {
    return config.relaysFlags.R01Man ? "checked" : "";
  }
  if (var == "R02MAN")
  {
    return config.relaysFlags.R02Man ? "checked" : "";
  }
  if (var == "R03MAN")
  {
    return config.relaysFlags.R03Man ? "checked" : "";
  }
  if (var == "R04MAN")
  {
    return config.relaysFlags.R04Man ? "checked" : "";
  }

  if (var == "VERSION_CODE")
  {
    return String(FPSTR(version));
  }
  if (var == "FECHA_COMPILACION")
  {
    return String(FPSTR(compile_date));
  }
  
  return String();
}

String processorOta(const String &var)
{
  if (var == "WORKING_MODE")
  {
    return workingModeString();
  }
  if (var == "VERSION_CODE")
  {
    char tmp[30];
    sprintf(tmp, "%s %s", version, beta);
    return String(tmp);
  }
  if (var == "FECHA_COMPILACION")
  {
    return String(FPSTR(compile_date));
  }
  
  return String();
}