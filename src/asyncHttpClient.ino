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
#define MAX_MESSAGE_SIZE 4999

static AsyncClient *aClient = NULL;
uint32_t connectionTimeout;
uint32_t receivingDataTimeout;
boolean receivingData = false;
boolean firstChunk = false;
uint8_t chunkNumber = 1;

uint8_t shellySensor = 1;

struct TCP_MESSAGE
{
  char message[MAX_MESSAGE_SIZE + 1];
  uint16_t totalMessageLength = 0;
  uint16_t messageLength = 0;
  uint16_t payloadStart = 0;
} message;

void runAsyncClient()
{
  if (config.flags.debug1) { INFOV(PSTR("\nFree Heap: %d bytes, Fragmentation: %.02f %%\n"), ESP.getFreeHeap(), getFragmentation()); }
  // Serial.printf("Connection timeout: %lu\n", millis() - connectionTimeout);
  
  if (processData) {
    if (config.flags.debug5) { INFOV(PSTR("Processing Received Data, waiting for a new request\n")); }
    return;
  }
  
  if (aClient) // client already exists
  { 
    if (config.flags.debug5) { INFOV(PSTR("Client already exists, waiting to finish the connection\n")); }

    if ((millis() - receivingDataTimeout) > RECEIVING_DATA_TIMEOUT) {
      Serial.printf("Processing Data timeout\n");
      receivingData = false;
    }

    if ((millis() - connectionTimeout) > CONNECTION_TIMEOUT && !receivingData) {
      Serial.printf("Closing connection by timeout\n");
      if (aClient->connected()) {
        aClient->close(true);
      } else { 
        clearMessage();
        deleteClient();
      }
    }
    return;
  }

  if (config.flags.debug5) { INFOV(PSTR("\nConnecting\n")); }
  
  aClient = new AsyncClient();
  if (!aClient) //could not allocate client
    return;
  
  connectionTimeout = millis();

  aClient->setRxTimeout(5);    // no RX data timeout for the connection in seconds

  aClient->onError([](void *arg, AsyncClient *client, err_t error) {
    if (config.flags.debug5) { INFOV(PSTR("Connect Error\n")); }
    receivingData = false;
    if (client->connected()) { client->close(true); }
  });

  aClient->onTimeout([](void *arg, AsyncClient *client, uint32_t time) {
    if (config.flags.debug5) { INFOV(PSTR("Timeout\n")); }
    receivingData = false;
    if (client->connected()) { client->close(true); }
  });

  aClient->onDisconnect([](void *arg, AsyncClient *client) {
    if (config.flags.debug5) { INFOV(PSTR("Disconnected\n\n")); }
    receivingData = false;
    delete client;
    aClient = NULL;

    if (message.messageLength > 0)
    {
      if (config.flags.debug5) { INFOV(PSTR("Parsing data\n")); }
      processData = true;
    }
  });

  aClient->onConnect([](void *arg, AsyncClient *client) {
    if (config.flags.debug5) { INFOV(PSTR("Connected\n")); }
    client->onError(NULL, NULL);

    static char url[250];

    switch (config.wversion)
    {
      case SOLAX_V2_LOCAL: // Solax v2 local mode
        // strcpy(url, "POST /?optType=ReadRealTimeData HTTP/1.1\r\nHost: 5.8.8.8\r\nConnection: close\r\nContent-Length: 0\r\nAccept: /*/\r\nContent-Type: application/x-www-form-urlencoded\r\nX-Requested-With: com.solaxcloud.starter\r\n\r\n");
        // strcpy(url, "POST /?optType=ReadRealTimeData&pwd=admin HTTP/1.1\r\nHost: 5.8.8.8\r\nConnection: close\r\nContent-Length: 0\r\nAccept: /*/\r\nContent-Type: application/x-www-form-urlencoded\r\nX-Requested-With: com.solaxcloud.starter\r\n\r\n");
        
        sprintf(url, "POST /?optType=ReadRealTimeData&pwd=admin HTTP/1.1\r\nHost: %s\r\nConnection: close\r\nContent-Length: 0\r\nAccept: /*/\r\nContent-Type: application/x-www-form-urlencoded\r\nX-Requested-With: com.solaxcloud.starter\r\n\r\n", config.sensor_ip);
        // sprintf(url, "POST /?optType=ReadRealTimeData&pwd=admin\r\n\r\n");
        break;
      case SOLAX_V1: // Solax v1
        sprintf(url, "GET /api/realTimeData.htm HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", config.sensor_ip);
        break;
      case WIBEEE: // Wibee
        sprintf(url, "GET /en/status.xml HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", config.sensor_ip);
        break;
      case SHELLY_EM: // Shelly EM
        sprintf(url, "GET /emeter/%d HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", (shellySensor - 1), config.sensor_ip);
        break;
      case FRONIUS_API: // Fronius
        sprintf(url, "GET /solar_api/v1/GetPowerFlowRealtimeData.fcgi HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", config.sensor_ip);
        break;
      case SLAVE_MODE: // Slave Freeds
        sprintf(url, "GET /masterdata HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", config.sensor_ip);
        break;
    }

    // send the request
    if (client->space() > 32 && client->canSend()) {
      client->write(url);
      if (config.flags.debug5) { INFOV(PSTR("Request send to %s\n"), IPAddress(client->getRemoteAddress()).toString().c_str()); }
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
    posContentLength = strstr(d, "Content-Length:", len, 15);
    if (posContentLength != NULL) {
      if (config.flags.debug5) { INFOV(PSTR("Content-Length received\n")); }
      
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
    }

    // Search for end of header
    dataPayload = strstr(d, "\r\n\r\n", len, 4);
    if (dataPayload != NULL) {
      uint16_t payloadPos = (uint16_t)(dataPayload - d + 4);
      if (config.flags.debug5) { INFOV(PSTR("End Header received in position %d\n"), payloadPos); }

      // If firstchunk is set, we clear all previous message
      if (firstChunk && payloadPos < 256) { INFOV(PSTR("End Header received twice, erasing previous message\n")); clearMessage(); }

      if (payloadPos < 256) {
        message.payloadStart = payloadPos;
        message.messageLength = (uint16_t)len - message.payloadStart;
      }
    }

    // Proccess the first chunk
    if (dataPayload != NULL && !firstChunk)
    {
      if (config.flags.debug5) { INFOV(PSTR("Received Data (Chunk number -> %d, Message Size: %d, Content-Length: %d, Data Size: %d, Payload Start: %d)\n"), chunkNumber, message.messageLength, message.totalMessageLength, (uint16_t)len, message.payloadStart); }
      
      firstChunk = true;
      arrayPos = 0;

      for (uint16_t i = message.payloadStart; i < len; i++)
      {
        message.message[arrayPos] = d[i];
        arrayPos++;
      }

    } else { // Proccess the next chunks

      chunkNumber++;
      arrayPos = message.messageLength;
      
      // If total message length is smaller than the buffer and the temp buffer + incoming data is smaller too, we proccess it.
      if (message.totalMessageLength < MAX_MESSAGE_SIZE && (message.messageLength + len) < MAX_MESSAGE_SIZE) {
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
      if (config.flags.debug5) { INFOV(PSTR("\nReceived Data (Chunk number -> %d, Message Size: %d, Content-Length: %d)\n"), chunkNumber, message.messageLength, message.totalMessageLength); }
    }

    if (config.flags.debug3) { Serial.printf(PSTR("\nMessage:\n%s\n"), d); }

    // If the message is fully received we close the connection
    if (message.messageLength == message.totalMessageLength)
    {
      if (config.flags.debug5) { INFOV(PSTR("Message completed\n")); }
      firstChunk = false;
      chunkNumber = 1;
      client->close();
    }
  });

  // if (config.wversion == SOLAX_V2_LOCAL) {
  //   if (!aClient->connect("5.8.8.8", 80))
  //   {
  //     if (config.flags.debug5) { INFOV(PSTR("Connect Fail\n")); }
  //     deleteClient();
  //   }
  // } else {
    if (!aClient->connect(String(config.sensor_ip).c_str(), 80))
    {
      if (config.flags.debug5) { INFOV(PSTR("Connect Fail\n")); }
      deleteClient();
    }
  // }
}

void deleteClient(void)
{
  // If client already exists, we delete it.
  if (aClient)
  { 
    Serial.printf(PSTR("Client not connected, deleting current client\n"));
    receivingData = false;
    AsyncClient *client = aClient;
    client->abort();
    delete client;
    aClient = NULL;
  }
}

void clearMessage(void)
{
  memset(message.message, 0, sizeof message.message);
  message.messageLength = 0;
  message.totalMessageLength = 0;
  firstChunk = false;
  chunkNumber = 1;
  receivingDataTimeout = millis();
}

void processingData(void)
{
  switch (config.wversion)
  {
    case SOLAX_V2_LOCAL: // Solax v2 local mode
      if (config.solaxVersion == 2) { parseJsonv2local(message.message); }
      else { parseJsonv3local(message.message); }
      break;
    case SOLAX_V1: // Solax v1
      parseJsonv1(message.message);
      break;
    case WIBEEE: // Wibee
      parseWibeee(message.message);
      break;
    case SHELLY_EM: // Shelly EM
      parseShellyEM(message.message, shellySensor);
      shellySensor++;
      if (shellySensor > 2) { shellySensor = 1; }
      break;
    case SLAVE_MODE:
      parseMasterFreeDs(message.message);
      break;
    case FRONIUS_API: // Fronius
      parseJsonFronius(message.message);
      break;
  }
  clearMessage();
  processData = false;
  receivingData = false;
}

// Function to implement strstr() function using KMP algorithm
inline char* strstr(char* X, const char* Y, int m, int n)
{
	// Base Case 1: Y is NULL or empty
	if (*Y == '\0' || n == 0)
		return X;

	// Base Case 2: X is NULL or X's length is less than that of Y's
	if (*X == '\0' || n > m)
		return NULL;

	// next[i] stores the index of next best partial match
	int next[n + 1];

	for (int i = 0; i < n + 1; i++)
		next[i] = 0;

	for (int i = 1; i < n; i++)
	{
		int j = next[i + 1];

		while (j > 0 && Y[j] != Y[i])
			j = next[j];

		if (j > 0 || Y[j] == Y[i])
			next[i + 1] = j + 1;
	}

	for (int i = 0, j = 0; i < m; i++)
	{
		if (*(X + i) == *(Y + j))
		{
			if (++j == n)
				return (X + i - j + 1);
		}
		else if (j > 0) {
			j = next[j];
			i--;	// // since i will be incremented in next iteration
		}
	}

	return NULL;
}