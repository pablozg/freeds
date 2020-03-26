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
  if (config.wversion == 0)
  {
    return "Solax Wifi v2 local";
  }
  if (config.wversion == 1)
  {
    return "Solax Wifi v1 - Hibridos";
  }
  if (config.wversion == 2)
  {
    return "Solax Wifi v2";
  }
  if (config.wversion == 3)
  {
    return "MQTT Broker";
  }
  if (config.wversion == 4)
  {
    return "Meter DDS238-2 Modbus";
  }
  if (config.wversion == 5)
  {
    return "Meter DDSU666 Modbus";
  }
  if (config.wversion == 6)
  {
    return "Meter SDM120 / SDM220 Modbus";
  }
  if (config.wversion == 9)
  {
    return "Wibee";
  }
  if (config.wversion == 10)
  {
    return "Shelly EM";
  }
  if (config.wversion == 11)
  {
    return "Fronius";
  }
  if (config.wversion == 12)
  {
    return "Slave FreeDS";
  }
  return String();
}

String processorFreeDS(const String &var)
{
  if (var == "SELECT_MODE")
  {
    return "<select class='form-control select2 mg-b-2 mg-r-30' onchange='run()' id='version'>"
                                                                              "<option value='1'" +
           String((config.wversion == 1) ? " selected='selected' " : " ") + ">Solax Wifi v1 - Hibridos</option>"
                                                                              "<option value='2'" +
           String((config.wversion == 2) ? " selected='selected' " : " ") + ">Solax Wifi v2</option>"
                                                                              "<option value='0'" +
           String((config.wversion == 0) ? " selected='selected' " : " ") + ">Solax Wifi v2 local</option>"
                                                                              "<option value='3'" +
           String((config.wversion == 3) ? " selected='selected' " : " ") + ">Solax MQTT (Tasmota)</option>"
                                                                              "<option value='4'" +
           String((config.wversion == 4) ? " selected='selected' " : " ") + ">Meter DDS238-2 Modbus</option>"
                                                                              "<option value='5'" +
           String((config.wversion == 5) ? " selected='selected' " : " ") + ">Meter DDSU666 Modbus</option>"
                                                                              "<option value='6'" +
           String((config.wversion == 6) ? " selected='selected' " : " ") + ">Meter SDM120 / SDM220 Modbus</option>"
                                                                              "<option value='11'" +
           String((config.wversion == 11) ? " selected='selected' " : " ") + ">Fronius</option>" +
                                                                              "<option value='9'" +
           String((config.wversion == 9) ? " selected='selected' " : " ") + ">Wibee</option>" +
                                                                              "<option value='10'" +                                                                   
           String((config.wversion == 10) ? " selected='selected' " : " ") + ">Shelly EM</option>"
                                                                              "<option value='12'" +
           String((config.wversion == 12) ? " selected='selected' " : " ") + ">Slave FreeDS</option></select>";
  }

  if (var == "MESSAGE")
  {
    switch (Message)
    {
    case 1:
      Message = 0;
      return "<div id='Info' class='alert alert-bordered alert-success' role='alert'>"
             "<button type='button' class='close' data-dismiss='alert' aria-label='Close'><span aria-hidden='true'>&times;</span></button>"
             "<div class='d-flex align-items-center justify-content-start'><i class='icon ion-ios-checkmark alert-icon tx-32 mg-t-5 mg-xs-t-0'></i>"
             "Configuración guardada correctamente.</div></div>";
      break;
    case 2:
      Message = 0;
      return "<div id='Info' class='alert alert-bordered alert-success' role='alert'>"
             "<button type='button' class='close' data-dismiss='alert' aria-label='Close'><span aria-hidden='true'>&times;</span></button>"
             "<div class='d-flex align-items-center justify-content-start'><i class='icon ion-ios-checkmark alert-icon tx-32 mg-t-5 mg-xs-t-0'></i>"
             "Reiniciando el dispositivo.</div></div>";
      break;
    }
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

String processorRed(const String &var)
{
  if (var == "WORKING_MODE")
  {
    return workingModeString();
  }
  if (var == "WIFI1")
  {
    String wifi = "<select id='wifi1' name='wifi1' class='form-control select2'><option disabled selected>Seleccione una red</option>";
                  for (int i = 0; i < 15; ++i) {
                    if (scanNetworks[i] == "") {break;}
                    if (scanNetworks[i] == String(config.ssid1)){ wifi +="<option value='" + scanNetworks[i] + "' selected>" + scanNetworks[i] + "</option>"; }
                    else { wifi +="<option value='" + scanNetworks[i] + "'>" + scanNetworks[i] + "</option>"; }
                  }
           wifi += "</select>";
    return wifi;
  }
  if (var == "PASS1") { return String(config.pass1); }
  if (var == "PASS2") { return String(config.pass2); }
  if (var == "WIFI2")
  {
    String wifi = "<select id='wifi2' name='wifi2' class='form-control select2'><option disabled selected>Seleccione una red</option>";
                  for (int i = 0; i < 15; ++i) {
                    if (scanNetworks[i] == "") {break;}
                    if (scanNetworks[i] == String(config.ssid2)){ wifi +="<option value='" + scanNetworks[i] + "' selected>" + scanNetworks[i] + "</option>"; }
                    else { wifi +="<option value='" + scanNetworks[i] + "'>" + scanNetworks[i] + "</option>"; }
                  }
           wifi += "</select>";
    return wifi;
  }
  if (var == "WIFIS")
  {
    if (config.wversion == 0)
    {
      return "<label id='labelModo' class='col-sm-4 form-control-label'>IP (Auto):</label>"
             "<div id='divModo' class='col-sm-8 mg-t-10 mg-sm-t-0'><input id='wifis' type=\"text\" class=\"form-control select2\" maxlength=\"30\" value=\"" +
             String(config.sensor_ip) + "\" name=\"wifis\" disabled /></div>";
    }
    if (config.wversion == 1)
    {
      return "<label id='labelModo' class='col-sm-4 form-control-label'>IP Wifi V1 solax:</label>"
             "<div id='divModo' class='col-sm-8 mg-t-10 mg-sm-t-0'><input id='wifis' type=\"text\" class=\"form-control select2\" maxlength=\"30\" value=\"" +
             String(config.sensor_ip) + "\" name=\"wifis\"/></div>";
    }
    if (config.wversion == 2)
    {
      String wifi = "<label id='labelModo' class='col-sm-4 form-control-label'>SSID Wifi inversor:</label>";
             wifi += "<div id='divModo' class='col-sm-8 mg-t-10 mg-sm-t-0'><select id='wifis' name='wifis' class='form-control select2'>";
             wifi += "<option disabled selected>Seleccione una red</option>";
                      for (int i = 0; i < 15; ++i) {
                        if (scanNetworks[i] == "") { break; }
                        if (scanNetworks[i] == String(config.ssid_esp01)){ wifi +="<option value='" + scanNetworks[i] + "' selected>" + scanNetworks[i] + "</option>"; }
                        else { wifi +="<option value='" + scanNetworks[i] + "'>" + scanNetworks[i] + "</option>"; }
                      }
             wifi += "</select></div>";
    return wifi;
    }
    if (config.wversion == 9)
    {
      return "<label id='labelModo' class='col-sm-4 form-control-label'>IP Wibeee:</label>"
             "<div id='divModo' class='col-sm-8 mg-t-10 mg-sm-t-0'><input id='wifis' type=\"text\" class=\"form-control select2\" maxlength=\"30\" value=\"" +
             String(config.sensor_ip) + "\" name=\"wifis\"/></div>";
    }
    if (config.wversion == 10)
    {
      return "<label id='labelModo' class='col-sm-4 form-control-label'>IP Shelly EM:</label>"
             "<div id='divModo' class='col-sm-8 mg-t-10 mg-sm-t-0'><input id='wifis' type=\"text\" class=\"form-control select2\" maxlength=\"30\" value=\"" +
             String(config.sensor_ip) + "\" name=\"wifis\"/></div>";
    }
    if (config.wversion == 11)
    {
      return "<label id='labelModo' class='col-sm-4 form-control-label'>IP Fronius:</label>"
             "<div id='divModo' class='col-sm-8 mg-t-10 mg-sm-t-0'><input id='wifis' type=\"text\" class=\"form-control select2\" maxlength=\"30\" value=\"" +
             String(config.sensor_ip) + "\" name=\"wifis\"/></div>";
    }
    if (config.wversion == 12)
    {
      return "<label id='labelModo' class='col-sm-4 form-control-label'>IP Master FreeDS:</label>"
             "<div id='divModo' class='col-sm-8 mg-t-10 mg-sm-t-0'><input id='wifis' type=\"text\" class=\"form-control select2\" maxlength=\"30\" value=\"" +
             String(config.sensor_ip) + "\" name=\"wifis\"/></div>";
    }
    // MQTT
    if (config.wversion == 3)
    {
      return "<label id='labelModo' class='col-sm-4 form-control-label'>MQTT Broker:</label>"
             "<div id='divModo' class='col-sm-8 mg-t-10 mg-sm-t-0'><input id='wifis' type=\"text\" class=\"form-control select2\" maxlength=\"30\" value=\"" +
             String(config.MQTT_broker) + "\" name=\"wifis\" disabled /></div>";
    }
    if (config.wversion == 4)
    {
      return "<label id='labelModo' class='col-sm-4 form-control-label'>Modbus:</label>"
             "<div id='divModo' class='col-sm-8 mg-t-10 mg-sm-t-0'><input id='wifis' type=\"text\" class=\"form-control select2\" maxlength=\"30\" value=\"DDS238-2\" name=\"wifis\" disabled /></div>";
    }
    if (config.wversion == 5)
    {
      return "<label id='labelModo' class='col-sm-4 form-control-label'>Modbus:</label>"
             "<div id='divModo' class='col-sm-8 mg-t-10 mg-sm-t-0'><input id='wifis' type=\"text\" class=\"form-control select2\" maxlength=\"30\" value=\"DDSU666\" name=\"wifis\" disabled /></div>";
    }
    if (config.wversion == 6)
    {
      return "<label id='labelModo' class='col-sm-4 form-control-label'>Modbus:</label>"
             "<div id='divModo' class='col-sm-8 mg-t-10 mg-sm-t-0'><input id='wifis' type=\"text\" class=\"form-control select2\" maxlength=\"30\" value=\"SDM120 / SDM 220\" name=\"wifis\" disabled /></div>";
    }
  }

  if (var == "HOST")
  {
    return String(config.hostServer);
  }
  if (var == "IP")
  {
    return config.dhcp ? WiFi.localIP().toString() : String(config.ip);
  }
  if (var == "GW")
  {
    return config.dhcp ? WiFi.gatewayIP().toString() : String(config.gw);
  }
  if (var == "MASK")
  {
    return config.dhcp ? WiFi.subnetMask().toString() : String(config.mask);
  }
  if (var == "DNS1")
  {
    return config.dhcp ? WiFi.dnsIP(0).toString() : String(config.dns1);
  }
  if (var == "DNS2")
  {
    return config.dhcp ? WiFi.dnsIP(1).toString() : String(config.dns2);
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
    return config.dhcp ? "checked" : "";
  }
  
  return String();
}

String processorConfig(const String &var)
{
  if (var == "WORKING_MODE")
  {
    return workingModeString();
  }
  if (var == "MQTT_ACTIVE")
  {
    return config.mqtt ? "checked" : "";
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
    if (config.wversion == 3)
    {
      return "<div class='row mg-t-2'><label class='col-sm-4 form-control-label'>Tema Solax:</label><div class='col-sm-8 mg-t-10 mg-sm-t-0'>"
             "<input id='solax' name='solax' type='text' class='form-control' maxlength='50' value='" +
             String(config.Solax_mqtt) + "'></div></div>"
                                           "<div class='row mg-t-2'><label class='col-sm-4 form-control-label'>Tema Meter:</label><div class='col-sm-8 mg-t-10 mg-sm-t-0'>"
                                           "<input id='meter' name='meter' type='text' class='form-control' maxlength='50' value='" +
             String(config.Meter_mqtt) + "'></div></div>";
    }
  }

  if (var == "STOREDPASS")
  {
    return String(config.password);
  }

  if (var == "REMOTE_API")
  {
    return String(config.remote_api);
  }

  if (var == "IDMETER")
  {
    return String(config.idMeter);
  }

  if (var == "AUTOPOWEROFF")
  {
    return config.oledAutoOff ? "checked" : "";
  }

  if (var == "AUTOPOWEROFFTIME")
  {
    return String(config.oledControlTime);
  }

  if (var == "MAXERRORTIME")
  {
    return String(config.maxErrorTime);
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
    return config.P01_on ? "checked" : "";
  }
  if (var == "PWMMIN")
  {
    return String(config.pwm_min);
  }
  if (var == "PWMMAX")
  {
    return String(config.pwm_max);
  }
  if (var == "LOOPPWM")
  {
    return String(config.pwmControlTime);
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
    return String(config.potmanpwm);
  }
  if (var == "POTPWMACTIVE")
  {
    return config.flags.potManPwmActive ? "checked" : "";
  }
  if (var == "AUTOPWM")
  {
    return String(config.autoControlPWM);
  }
  if (var == "R01MIN")
  {
    return String(config.R01_min);
  }
  if (var == "R01POTON")
  {
    return String(config.R01_poton);
  }
  if (var == "R01POTOFF")
  {
    return String(config.R01_potoff);
  }
  if (var == "R02MIN")
  {
    return String(config.R02_min);
  }
  if (var == "R02POTON")
  {
    return String(config.R02_poton);
  }
  if (var == "R02POTOFF")
  {
    return String(config.R02_potoff);
  }
  if (var == "R03MIN")
  {
    return String(config.R03_min);
  }
  if (var == "R03POTON")
  {
    return String(config.R03_poton);
  }
  if (var == "R03POTOFF")
  {
    return String(config.R03_potoff);
  }
  if (var == "R04MIN")
  {
    return String(config.R04_min);
  }
  if (var == "R04POTON")
  {
    return String(config.R04_poton);
  }
  if (var == "R04POTOFF")
  {
    return String(config.R04_potoff);
  }

  if (var == "R01MAN")
  {
    return config.R01_man ? "checked" : "";
  }
  if (var == "R02MAN")
  {
    return config.R02_man ? "checked" : "";
  }
  if (var == "R03MAN")
  {
    return config.R03_man ? "checked" : "";
  }
  if (var == "R04MAN")
  {
    return config.R04_man ? "checked" : "";
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
    return String(FPSTR(version));
  }
  if (var == "FECHA_COMPILACION")
  {
    return String(FPSTR(compile_date));
  }
  
  return String();
}