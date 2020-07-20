/*
  asyncHttpClient.ino - FreeDs http async client
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

#define CONNECTION_TIMEOUT 30000
#define RECEIVING_DATA_TIMEOUT 10000

static AsyncClient *aClient = NULL;
uint32_t connectionTimeout;
uint32_t receivingDataTimeout;
boolean receivingData = false;
boolean firstChunk = false;

uint8_t shellySensor = 1;

struct TCP_MESSAGE
{
  char message[4000];
  uint16_t totalMessageLength = 0;
  uint16_t messageLength = 0;
  uint16_t payloadStart = 0;
} message;

void runAsyncClient()
{
  INFOV("\nFree Heap: %d bytes, Fragmentation: %.02f %%\n", ESP.getFreeHeap(), getFragmentation());
  Serial.printf("Connection timeout: %lu\n", millis() - connectionTimeout);
  
  if (processData) {
    if (config.flags.moreDebug) { INFOV("Processing Received Data, waiting for a new request\n"); }
    return;
  }
  
  if (aClient) // client already exists
  { 
    if (config.flags.moreDebug) { INFOV("Client already exists, waiting to finish the connection\n"); }

    if ((millis() - receivingDataTimeout) > RECEIVING_DATA_TIMEOUT) {
      Serial.printf("Processing Data timeout\n");
      receivingData = false;
    }

    if ((millis() - connectionTimeout) > CONNECTION_TIMEOUT && !receivingData) {
      Serial.printf("Cerrando conexión por timeout\n");
      if (aClient->connected()) {
        aClient->close(true);
      } else { 
        clearMessage();
        deleteClient();
      }
    }
    return;
  }

  if (config.flags.moreDebug) { INFOV("Connecting\n"); }
  
  aClient = new AsyncClient();
  if (!aClient) //could not allocate client
    return;
  
  connectionTimeout = millis();

  aClient->setRxTimeout(3);    // no RX data timeout for the connection in seconds

  aClient->onError([](void *arg, AsyncClient *client, err_t error) {
    if (config.flags.moreDebug) { INFOV("Connect Error\n"); }
    receivingData = false;
    if (client->connected()) { client->close(true); }
  });

  aClient->onTimeout([](void *arg, AsyncClient *client, uint32_t time) {
    if (config.flags.moreDebug) { INFOV("Timeout\n"); }
    receivingData = false;
    if (client->connected()) { client->close(true); }
  });

  aClient->onDisconnect([](void *arg, AsyncClient *client) {
    if (config.flags.moreDebug) { INFOV("Disconnected\n\n"); }
    receivingData = false;
    delete client;
    aClient = NULL;

    if (message.messageLength > 0)
    {
      if (config.flags.moreDebug) { INFOV("Parsing data\n"); }
      processData = true;
    }
  });

  aClient->onConnect([](void *arg, AsyncClient *client) {
    if (config.flags.moreDebug) { INFOV("Connected\n"); }
    client->onError(NULL, NULL);

    static char url[250];

    switch (config.wversion)
    {
      case 0: // Solax v2 local mode
        strcpy(url, "POST /?optType=ReadRealTimeData HTTP/1.1\r\nHost: 5.8.8.8\r\nConnection: close\r\nContent-Length: 0\r\nAccept: /*/\r\nContent-Type: application/x-www-form-urlencoded\r\nX-Requested-With: com.solaxcloud.starter\r\n\r\n");
        break;
      case 1: // Solax v1
        sprintf(url, "GET /api/realTimeData.htm HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", config.sensor_ip);
        break;
      case 9: // Wibee
        sprintf(url, "GET /en/status.xml HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", config.sensor_ip);
        break;
      case 10: // Shelly EM
        sprintf(url, "GET /emeter/%d HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", (shellySensor - 1), config.sensor_ip);
        break;
      case 11: // Fronius
        sprintf(url, "GET /solar_api/v1/GetPowerFlowRealtimeData.fcgi HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", config.sensor_ip);
        break;
      case 12: // Slave Freeds
        sprintf(url, "GET /masterdata HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", config.sensor_ip);
        break;
    }

    // send the request
    if (client->space() > 32 && client->canSend()) {
      client->write(url);
      if (config.flags.moreDebug) { INFOV("Request send to %s\n", IPAddress(client->getRemoteAddress()).toString().c_str()); }
    }
  });

  aClient->onData([](void *arg, AsyncClient *client, void *data, size_t len) {      
    char tmp[6];
    char *posContentLength;
    char *dataPayload;
    char *d = (char *)data;
    uint16_t arrayPos = 0;
   
    // Set flags and timeout
    receivingData = true;
    receivingDataTimeout = millis();
    
    // Search for content length
    posContentLength = strstr(d, "Content-Length:");
    if (posContentLength != NULL) {
      if (config.flags.moreDebug) { INFOV("Content-Length received\n"); }
      
      // If firstchunk is set, we clear all previous message
      if (firstChunk) { clearMessage(); }

      uint16_t contentLengthStart = posContentLength - d;
      posContentLength  = strchr(posContentLength + 1,'\r');
      uint16_t contentLengthStop = posContentLength - d;
      
      // Store the content length
      for (uint16_t i = contentLengthStart + 16; i < contentLengthStop; i++ )
      {
        tmp[arrayPos] = d[i];
        arrayPos++;
      }
      message.totalMessageLength = atoi(tmp);
    } else { if (config.flags.moreDebug) { INFOV("Content-Length is null\n"); } }

    // Search for end of header
    dataPayload = strstr(d, "\r\n\r\n");
    if (dataPayload != NULL && !firstChunk) {
      if (config.flags.moreDebug) { INFOV("End Header received\n"); }
      message.payloadStart = (uint16_t)(dataPayload - d + 4);
      message.messageLength = (uint16_t)len - message.payloadStart;
      
      // If bad data is received, set the payload start as the total lenght of message
      if (message.payloadStart > len) { 
        message.payloadStart = (uint16_t)len;
        message.messageLength = 0;
      }
    } else { if (config.flags.moreDebug) { INFOV("End Header is null or first chunk received previously\n"); } }

    // Proccess the first chunk
    if (dataPayload != NULL && !firstChunk)
    {
      if (config.flags.moreDebug) { INFOV("First chunk\nLen: %d, payloadStart: %d, messageLength: %d\n", (uint16_t)len, message.payloadStart, message.messageLength); }
      
      firstChunk = true;
      arrayPos = 0;

      for (uint16_t i = message.payloadStart; i < len; i++)
      {
        message.message[arrayPos] = d[i];
        arrayPos++;
      }

    } else { // Proccess the next chunks

      if (config.flags.moreDebug) { INFOV("Next chunk\n"); }
      
      arrayPos = message.messageLength;
      
      // If total message length is smaller than the buffer, we proccess it.
      if (message.totalMessageLength < 3999) {
        for (uint16_t i = 0; i < len; i++)
        {
          message.message[arrayPos] = d[i];
          arrayPos++;
        }
        message.message[arrayPos] = '\0';
        message.messageLength += (uint16_t)len;
      } else {
        clearMessage();
        receivingData = false;
        client->close();
      }
    }

    INFOV("Datos Recibidos, message: %d, total: %d\n", message.messageLength, message.totalMessageLength);
    if (config.flags.messageDebug) { Serial.printf("Message:%s\n", d); }

    // If the message is fully received we close the connection
    if (message.messageLength == message.totalMessageLength)
    {
      if (config.flags.moreDebug) { INFOV("Message fully received\n"); }
      client->close();
    }
  });

  if (config.wversion == 0) {
    if (!aClient->connect("5.8.8.8", 80))
    {
      if (config.flags.moreDebug) { INFOV("Connect Fail\n"); }
      deleteClient();
    }
  } else {
    if (!aClient->connect(String(config.sensor_ip).c_str(), 80))
    {
      if (config.flags.moreDebug) { INFOV("Connect Fail\n"); }
      deleteClient();
    }
  }
}

void deleteClient(void)
{
  // If client already exists, we delete it.
  if (aClient)
  { 
    Serial.printf("Cliente no conectado, eliminado cliente creado\n");
    receivingData = false;
    AsyncClient *client = aClient;
    client->abort();
    delete client;
    aClient = NULL;
    Serial.printf("Cliente no conectado, despues de null\n");
  }
}

void clearMessage(void)
{
  memset(message.message, 0, sizeof message.message);
  message.messageLength = 0;
  message.totalMessageLength = 0;
  firstChunk = false;
}

void processingData(void)
{
  switch (config.wversion)
  {
    case 0: // Solax v2 local mode
      parseJsonv2local(message.message);
      break;
    case 1: // Solax v1
      parseJsonv1(message.message);
      break;
    case 9: // Wibee
      parseWibeee(message.message);
      break;
    case 10: // Shelly EM
      parseShellyEM(message.message, shellySensor);
      shellySensor++;
      if (shellySensor > 2) { shellySensor = 1; }
      break;
    case 12:
      parseMasterFreeDs(message.message);
      break;
    case 11: // Fronius
      parseJson_fronius(message.message);
      break;
  }
  clearMessage();
  processData = false;
  receivingData = false;
}