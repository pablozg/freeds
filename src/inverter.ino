void sendStatusSolaxV2(void)
{
  DEBUGLN("\r\nSENDSTATUSSOLAXV2()");

  if (config.wversion == 2) { SerieEsp.println("###STATUS"); }
}

// Solax v2
void m1_com(void)
{
  if (SerieEsp.available())
  {
    String currentLine;
    currentLine = SerieEsp.readStringUntil('\n');
    if (currentLine.startsWith("###VERSION"))
    {
      esp01_version = currentLine.substring(currentLine.indexOf("###VERSION") + 14, currentLine.indexOf("$"));
    }
    if (currentLine.startsWith("###JSONERROR"))
    {
      INFOLN("-----M1: Error decodificando JSON");
    }
    if (currentLine.startsWith("###STATUS"))
    {
      String buf;
      buf = currentLine.substring(currentLine.indexOf("###STATUS:") + 10, currentLine.indexOf("$$$"));
      esp01_status = buf.toInt();
      if (esp01_status == 0)
        SerieEsp.println("###SSID=" + String(config.ssid_esp01) + "$$$");
      else
      {
        errorConexionInversor = false;
        temporizadorErrorConexionRed = millis();
      }
    }
    if (currentLine.startsWith("###PAYLOAD"))
    {
      errorConexionInversor = false;
      esp01_payload = currentLine.substring(currentLine.indexOf("{\"Data\""), currentLine.indexOf("$$$"));
      parseJson(esp01_payload);
    }
    if (currentLine.startsWith("##D M1: NOT CONNECT"))
    {
      INFOLN("-----M1: Mo conectado a inversor");
      errorConexionInversor = true;
    }
    if (currentLine.startsWith("###HTTPCODE"))
    {
      String buf;
      buf = currentLine.substring(currentLine.indexOf("###HTTPCODE:") + 12, currentLine.indexOf("$$$"));
      httpcode = buf.toInt();
    }
    if (currentLine.startsWith("##D"))
    { 
      DEBUGLN(currentLine.substring(currentLine.indexOf("##D") + 3, currentLine.indexOf("\n")));
    }
  }
}

// Solax v1
void v1_com(void)
{
    HTTPClient clientHttp;
    WiFiClient clientWifi;
    clientHttp.setConnectTimeout(3000);
    httpcode = -1;
    
    String url = "http://" + (String)config.sensor_ip + "/api/realTimeData.htm";
    clientHttp.begin(clientWifi, url);
    httpcode = clientHttp.GET();

    DEBUGLN("HTTPCODE ERROR: " + (String)httpcode);

    if (httpcode == HTTP_CODE_OK)
    {
      String Resp = clientHttp.getString();
      parseJsonv1(Resp);
      errorConexionInversor = false;
    }
    clientHttp.end();
    clientWifi.stop();
}

// Solax v2 local
void v0_com(void)
{ 
    HTTPClient clientHttp;
    WiFiClient clientWifi;
    clientHttp.setConnectTimeout(3000);
    httpcode = -1;

    clientHttp.begin(clientWifi, "http://5.8.8.8/?optType=ReadRealTimeData");
    clientHttp.addHeader("Host", "5.8.8.8");
    clientHttp.addHeader("Content-Length", "0");
    clientHttp.addHeader("Accept", "/*/");
    clientHttp.addHeader("Content-Type", "application/x-www-form-urlencoded");
    clientHttp.addHeader("X-Requested-With", "com.solaxcloud.starter");

    httpcode = clientHttp.POST("");

    DEBUGLN("HTTPCODE ERROR: " + (String)httpcode);

    if (httpcode == HTTP_CODE_OK)
    {
      String Resp = clientHttp.getString();
      parseJsonv1(Resp);
      errorConexionInversor = false;
    }
    clientHttp.end();
    clientWifi.stop();
}

// Fronius
void fronius_com(void)
{  
    HTTPClient clientHttp;
    WiFiClient clientWifi;
    clientHttp.setConnectTimeout(4000);
    httpcode = -1;
    String url = "http://" + (String)config.sensor_ip + "/solar_api/v1/GetPowerFlowRealtimeData.fcgi";
    clientHttp.begin(clientWifi, url);
    httpcode = clientHttp.GET();

    DEBUGLN("HTTPCODE ERROR: " + (String)httpcode);

    if (httpcode == HTTP_CODE_OK)
    {
      String Resp = clientHttp.getString();
      INFOLN("JSON STRING: " + Resp);
      parseJson_fronius(Resp);
      errorConexionInversor = false;
    }
    clientHttp.end();
    clientWifi.stop();
}

// Solax v2
void parseJson(String json)
{
  DEBUGLN("JSON:" + json);
  DeserializationError error = deserializeJson(root, json);
  
  if (error)  {
    INFO("deserializeJson() failed: ");
    INFOLN(error.c_str());
    httpcode = -1;
  } else {
    DEBUGLN("deserializeJson() OK");

    httpcode = root["Data"][0];          // Error code
    inverter.pv1c = root["Data"][1];     // Corriente string 1
    inverter.pv2c = root["Data"][2];     // Corriente string 2
    inverter.pv1v = root["Data"][3];     // Tension string 1
    inverter.pv2v = root["Data"][4];     // Tension string 2
    inverter.pw1 = root["Data"][5];      // Potencia string 1
    inverter.pw2 = root["Data"][6];      // Potencia string 2
    inverter.gridv = root["Data"][7];    // Tension de red
    inverter.wsolar = root["Data"][8];   // Potencia solar
    inverter.wtoday = root["Data"][9];   // Potencia solar diaria
    inverter.wgrid = root["Data"][10];   // Potencia de red (Negativo: de red - Positivo: a red)
    inverter.wtogrid = root["Data"][11]; // Potencia diaria enviada a red

    errorConexionInversor = false;
    temporizadorErrorConexionRed = millis();
  }
}

// Solax v1 y v2 local
void parseJsonv1(String json)
{
  DEBUGLN("JSON:" + json);
  DeserializationError error = deserializeJson(root, json);
  
  if (error) {
    INFO("deserializeJson() failed: ");
    INFOLN(error.c_str());
  } else {
    DEBUGLN("deserializeJson() OK");

    inverter.pv1c = root["Data"][0];     // Corriente string 1
    inverter.pv2c = root["Data"][1];     // Corriente string 2
    inverter.pv1v = root["Data"][2];     // Tension string 1
    inverter.pv2v = root["Data"][3];     // Tension string 2
    inverter.gridv = root["Data"][5];    // Tension de red
    inverter.wsolar = root["Data"][6];   // Potencia solar
    inverter.wtoday = root["Data"][8];   // Potencia solar diaria
    inverter.wgrid = root["Data"][10];   // Potencia de red (Negativo: de red - Positivo: a red)
    inverter.pw1 = root["Data"][11];     // Potencia string 1
    inverter.pw2 = root["Data"][12];     // Potencia string 2
    inverter.wtogrid = root["Data"][41]; // Potencia diaria enviada a red

    errorConexionInversor = false;
    temporizadorErrorConexionRed = millis();
  }
}

// Fronius
void parseJson_fronius(String json)
{
  DEBUGLN("JSON:" + json);
  DeserializationError error = deserializeJson(root, json);
  
  if (error) {
    INFO("deserializeJson() failed: ");
    INFOLN(error.c_str());
    httpcode = -1;
  } else {
    DEBUGLN("deserializeJson() OK");
    inverter.wsolar = root["Body"]["Data"]["Site"]["P_PV"] == "null" ? 0 : root["Body"]["Data"]["Site"]["P_PV"];     // Potencia solar
    inverter.wtoday = root["Body"]["Data"]["Site"]["E_Day"] == "null" ? 0 : root["Body"]["Data"]["Site"]["E_Day"];   // Potencia solar diaria
    inverter.wgrid  = root["Body"]["Data"]["Site"]["P_Grid"] == "null" ? 0 : root["Body"]["Data"]["Site"]["P_Grid"]; // Potencia de red (Negativo: de red - Positivo: a red)
    inverter.wgrid  *= -1;
    inverter.wtoday = inverter.wtoday / 1000; // w->Kw
    
    errorConexionInversor = false;
    temporizadorErrorConexionRed = millis();
  }
}