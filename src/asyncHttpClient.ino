#define CONNECTION_TIMEOUT 10000

static AsyncClient *aClient = NULL;
uint32_t connectionTimeout;
boolean processingData = false;

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
  INFOV("\nFree Heap: %d bytes\n\n", ESP.getFreeHeap());
  //Serial.printf("Connection timeout: %lu\n", millis() - connectionTimeout);
  
  if (aClient) //client already exists
  { 
    if (config.flags.moreDebug) { INFOV("\nClient already exists\n"); }
    if ((millis() - connectionTimeout) > CONNECTION_TIMEOUT && aClient->connected() && !processingData) {
      Serial.printf("Cerrando conexión por timeout\n");
      checkClientStatus();
    }
    return;
  }

  if (config.flags.moreDebug) { INFOV("\nConnecting\n"); }
  
  aClient = new AsyncClient();
  if (!aClient) //could not allocate client
    return;
  
  connectionTimeout = millis();

  aClient->setRxTimeout(4);    // no RX data timeout for the connection in seconds

  aClient->onError([](void *arg, AsyncClient *client, err_t error) {
    if (config.flags.moreDebug) { INFOV("\nConnect Error\n"); }
    client->close(true);
  });

  aClient->onTimeout([](void *arg, AsyncClient *client, uint32_t time) {
    if (config.flags.moreDebug) { INFOV("\nTimeout\n"); }
    client->close(true);
  });

  aClient->onDisconnect([](void *arg, AsyncClient *client) {
    if (config.flags.moreDebug) { INFOV("\nDisconnected\n"); }
    processingData = false;
    delete client;
    aClient = NULL;
  });

  aClient->onConnect([](void *arg, AsyncClient *client) {
    if (config.flags.moreDebug) { INFOV("\nConnected\n"); }
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
      if (config.flags.debug) { INFOV("\nRequest send to %s\n", IPAddress(client->getRemoteAddress()).toString().c_str()); }
    }
  });

  aClient->onData([](void *arg, AsyncClient *client, void *data, size_t len) {      
    char tmp[6];
    char *posContentLength;
    char *dataPayload;
    char *d = (char *)data;
    uint16_t arrayPos = 0;
   
    processingData = true;
    
    posContentLength = strstr(d, "Content-Length:");
    if (posContentLength != NULL) {
      uint16_t contentLengthStart = posContentLength - d;
      posContentLength  = strchr(posContentLength + 1,'\r');
      uint16_t contentLengthStop = posContentLength - d;
      
      for (uint16_t i = contentLengthStart + 16; i < contentLengthStop; i++ )
      {
        tmp[arrayPos] = d[i];
        arrayPos++;
      }
      message.totalMessageLength = atoi(tmp);
    } else { if (config.flags.moreDebug) { INFOV("Content-Length is null\n"); } }


    dataPayload = strstr(d, "\r\n\r\n");
    if (dataPayload != NULL) {
      message.payloadStart = (uint16_t)(dataPayload - d + 4);
      message.messageLength = (uint16_t)len - message.payloadStart;
      // If bad data is received, we miss the data
      if (message.payloadStart > len) { 
        message.payloadStart = (uint16_t)len;
        message.messageLength = 0;
      }
    } else { if (config.flags.moreDebug) { INFOV("End Header is null\n"); } }

    if (dataPayload != NULL) // Proccess the first chunk                  
    {
      if (config.flags.moreDebug) { INFOV("\n\nFirst chunk\nLen: %d, payloadStart: %d, messageLength: %d\n", (uint16_t)len, message.payloadStart, message.messageLength); }

      arrayPos = 0;
      for (uint16_t i = message.payloadStart; i < len; i++)
      {
        message.message[arrayPos] = d[i];
        arrayPos++;
      }
      
    } else { // Proccess the next chunks

      arrayPos = message.messageLength;
      
      for (uint16_t i = 0; i < len; i++)
      {
        message.message[arrayPos] = d[i];
        arrayPos++;
      }
      message.message[arrayPos] = '\0';
      message.messageLength += (uint16_t)len;
      if (config.flags.moreDebug) { INFOV("Next chunk\n"); }
    }

    INFOV("\nDatos Recibidos, message: %d, total: %d\n", message.messageLength, message.totalMessageLength);
    if (config.flags.moreDebug) { Serial.printf("Menssage:%s",d); }

    if (message.messageLength == message.totalMessageLength || (message.totalMessageLength == 0 && (uint16_t)len < 1436))
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
      memset(message.message, 0, sizeof message.message);
      message.messageLength = 0;
      message.totalMessageLength = 0;
      client->close();
    }
  });

  if (config.wversion == 0) {
    if (!aClient->connect("5.8.8.8", 80))
    {
      if (config.flags.moreDebug) { INFOV("\nConnect Fail\n"); }
      checkClientStatus();
    }
  } else {
    if (!aClient->connect(String(config.sensor_ip).c_str(), 80))
    {
      if (config.flags.moreDebug) { INFOV("\nConnect Fail\n"); }
      checkClientStatus();
    }
  }
}

void checkClientStatus(void)
{
  if (aClient) //If client already exists, we delete it.
  { 
    if (aClient->connected()) { aClient->close(true); }
    processingData = false;
    AsyncClient *client = aClient;
    delete client;
    aClient = NULL;
  }
}