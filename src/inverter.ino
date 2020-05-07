/*
  inverter.ino - FreeDs inverter functions
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

void sendStatusSolaxV2(void)
{
  if (config.wversion == 2) { 
    
    if (config.flags.moreDebug) { INFOV("SENDSTATUSSOLAXV2()\n"); }
    SerieEsp.println("###STATUS");
  }
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
      INFOV("-----M1: Error decodificando JSON\n");
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
        Error.ConexionInversor = false;
        timers.ErrorConexionRed = millis();
      }
    }
    if (currentLine.startsWith("###PAYLOAD"))
    {
      Error.ConexionInversor = false;
      esp01_payload = currentLine.substring(currentLine.indexOf("{\"Data\""), currentLine.indexOf("$$$"));
      parseJson(esp01_payload);
    }
    if (currentLine.startsWith("##D M1: NOT CONNECT"))
    {
      INFOV("-----M1: Mo conectado a inversor\n");
      Error.ConexionInversor = true;
    }
    if (currentLine.startsWith("###HTTPCODE"))
    {
      String buf;
      buf = currentLine.substring(currentLine.indexOf("###HTTPCODE:") + 12, currentLine.indexOf("$$$"));
      httpcode = buf.toInt();
    }
    if (currentLine.startsWith("##D"))
    { 
      if (config.flags.moreDebug) { INFOV("%s\n",currentLine.substring(currentLine.indexOf("##D") + 3, currentLine.indexOf("\n")).c_str()); }
    }
  }
}

// Solax v2
void parseJson(String json)
{
  if (config.flags.debugOutput) { INFOV("Size: %lu, Json: %s\n", strlen(json.c_str()), json.c_str()); }
  
  DeserializationError error = deserializeJson(root, json);
  
  if (error)  {
    INFOV("deserializeJson() failed: %s\n", error.c_str());
    httpcode = -1;
  } else {
    
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

    Error.ConexionInversor = false;
    timers.ErrorConexionRed = millis();
  }
}

// Solax v1
void parseJsonv1(String json)
{
  if (config.flags.debugOutput) { INFOV("Size: %lu, Json: %s\n", strlen(json.c_str()), json.c_str()); }
  uint16_t start = json.indexOf(":[");
  uint16_t stop = json.indexOf(",", start);
  float res[14];

  res[0] = json.substring(start + 2, stop).toFloat(); 

  for (int i = 1; i <= 41; i++)
  {
    start = json.indexOf(",", stop);
    stop = json.indexOf(",", start + 1);
    if (i <= 12) { res[i] = json.substring(start + 1, stop).toFloat(); }
    if (i == 41) { res[13] = json.substring(start + 1, stop).toFloat(); }
  }
  
    inverter.pv1c = res[0];     // Corriente string 1
    inverter.pv2c = res[1];     // Corriente string 2
    inverter.pv1v = res[2];     // Tension string 1
    inverter.pv2v = res[3];     // Tension string 2
    inverter.gridv = res[5];    // Tension de red
    inverter.wsolar = res[6];   // Potencia solar
    inverter.wtoday = res[8];   // Potencia solar diaria
    inverter.wgrid = res[10];   // Potencia de red (Negativo: de red - Positivo: a red)
    inverter.pw1 = res[11];     // Potencia string 1
    inverter.pw2 = res[12];     // Potencia string 2
    inverter.wtogrid = res[13]; // Potencia diaria enviada a red

    Error.ConexionInversor = false;
    timers.ErrorConexionRed = millis();
}

// Solax v2 local
void parseJsonv2local(String json)
{
  if (config.flags.debugOutput) { INFOV("Size: %lu, Json: %s\n", strlen(json.c_str()), json.c_str()); }
  
  DeserializationError error = deserializeJson(root, json);
  
  if (error) {
    INFOV("deserializeJson() failed: %s\n", error.c_str());
  } else {

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

    Error.ConexionInversor = false;
    timers.ErrorConexionRed = millis();
  }
}

// Fronius
void parseJson_fronius(String json)
{
  if (config.flags.debugOutput) { INFOV("Size: %lu, Json: %s\n", strlen(json.c_str()), json.c_str()); }
  DeserializationError error = deserializeJson(root, json);
  
  if (error) {
    INFOV("deserializeJson() failed: %s\n", error.c_str());
    httpcode = -1;
  } else {
    inverter.wsolar = root["Body"]["Data"]["Site"]["P_PV"] == "null" ? 0 : root["Body"]["Data"]["Site"]["P_PV"];     // Potencia solar
    inverter.wtoday = root["Body"]["Data"]["Site"]["E_Day"] == "null" ? 0 : root["Body"]["Data"]["Site"]["E_Day"];   // Potencia solar diaria
    inverter.wgrid  = root["Body"]["Data"]["Site"]["P_Grid"] == "null" ? 0 : root["Body"]["Data"]["Site"]["P_Grid"]; // Potencia de red (Negativo: de red - Positivo: a red)
    inverter.wgrid  *= -1;
    inverter.wtoday = inverter.wtoday / 1000; // w->Kw
    
    Error.ConexionInversor = false;
    timers.ErrorConexionRed = millis();
  }
}