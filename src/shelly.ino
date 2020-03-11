void getShellyData(void)
{
  DEBUGLN("\r\nGETSHELLYDATA()");

  if (config.wifi)
  {
    if (config.wversion == 10)
    {
      for (int sensor = 1; sensor <= 2; sensor++)
      {
        shellyCom(sensor); // Shelly EM
      }
    }
  }
}

// Shelly EM
void shellyCom(int sensor)
{
    HTTPClient clientHttp;
    WiFiClient clientWifi;
    clientHttp.setConnectTimeout(4000);
    httpcode = -1;
    
    String url = "http://" + (String)config.sensor_ip + "/emeter/" + (String)(sensor - 1);
    clientHttp.begin(clientWifi, url);
    httpcode = clientHttp.GET();

    INFOLN("HTTPCODE ERROR: " + (String)httpcode + " Sensor: " + (String)(sensor - 1));

    if (httpcode == HTTP_CODE_OK)
    {
      String Resp = clientHttp.getString();
      parseShellyEM(Resp, sensor);
      errorConexionInversor = false;
    }
    clientHttp.end();
    clientWifi.stop();
}

// Shelly EM
void parseShellyEM(String json, int sensor)
{
  DEBUGLN("JSON:" + json);
  DeserializationError error = deserializeJson(root, json);
  
  if (error) {
    INFO("deserializeJson() failed: ");
    INFOLN(error.c_str());
    httpcode = -1;
  } else {
    DEBUGLN("deserializeJson() OK");
    switch (sensor)
    {
      case 1: // Medida de Red
        if (root["is_valid"] == true) {
          meter.activePower = inverter.wgrid = (int)root["power"] * -1; // Potencia de red (Negativo: de red - Positivo: a red)
          meter.voltage = root["voltage"];
          meter.reactivePower = (int)root["reactive"] * -1;
          meter.importActive = root["total"];
          meter.exportActive = root["total_returned"];
          errorConexionInversor = false;
        }
        break;
      case 2: // Medida de Inversor
        if (root["is_valid"] == true) {
          inverter.wsolar = (int)root["power"] * -1; // Potencia de red (Negativo: de red - Positivo: a red)
          inverter.gridv = root["voltage"];
          errorConexionInversor = false;
        }
        break;
    }
    temporizadorErrorConexionRed = millis();
  }
}