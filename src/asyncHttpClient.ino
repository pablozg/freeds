static AsyncClient *aClient = NULL;

uint8_t shellySensor = 1;

struct TCP_MESSAGE
{
    String message = "";
    uint16_t totalMessageLength = 0;
    uint16_t messageLength = 0;
    uint16_t headerLength = 0;
} message;

void runAsyncClient()
{
    //Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
    if (aClient) //client already exists
    { 
        if (config.flags.moreDebug) { INFOV("client already exists\n"); }
        aClient->close(true);
        return;
    }

    aClient = new AsyncClient();
    if (!aClient) //could not allocate client
        return;

    aClient->onError([](void *arg, AsyncClient *client, err_t error) {
        if (config.flags.moreDebug) { INFOV("Connect Error\n"); }
        client->close(true);
    });

    aClient->onTimeout([](void *arg, AsyncClient *client, uint32_t time) {
        if (config.flags.moreDebug) { INFOV("Timeout\n"); }
        client->close(true);
    });

    aClient->onDisconnect([](void *arg, AsyncClient *client) {
        if (config.flags.moreDebug) { INFOV("Disconnected\n"); }
        client->free();
        delete client;
        aClient = NULL;
    });

    aClient->onConnect([](void *arg, AsyncClient *client) {
        if (config.flags.moreDebug) { INFOV("Connected\n"); }
        client->onError(NULL, NULL);
        client->setRxTimeout(2);    //no RX data timeout for the connection in seconds
        client->setAckTimeout(300); //no ACK timeout for the last sent packet in milliseconds

        String url;

        switch (config.wversion)
        {
        case 0: // Solax v2 local mode
            url = "POST /?optType=ReadRealTimeData HTTP/1.1\r\nHost: 5.8.8.8\r\nConnection: close\r\nContent-Length: 0\r\nAccept: /*/\r\nContent-Type: application/x-www-form-urlencoded\r\nX-Requested-With: com.solaxcloud.starter\r\n\r\n";
            break;
        case 1: // Solax v1
            url = "GET /api/realTimeData.htm HTTP/1.1\r\nHost: " + (String)config.sensor_ip + "\r\nConnection: close\r\n\r\n";
            break;
        case 9: // Wibee
            url = "GET /en/status.xml HTTP/1.1\r\nHost: " + (String)config.sensor_ip + "\r\nConnection: close\r\n\r\n";
            break;
        case 10: // Shelly EM
            url = "GET /emeter/" + (String)(shellySensor - 1) + " HTTP/1.1\r\nHost: " + (String)config.sensor_ip + "\r\nConnection: close\r\n\r\n";
            break;
        case 11: // Fronius
            url = "GET /solar_api/v1/GetPowerFlowRealtimeData.fcgi HTTP/1.1\r\nHost: " + (String)config.sensor_ip + "\r\nConnection: close\r\n\r\n";
            break;
         case 12: // Slave Freeds
            url = "GET /masterdata HTTP/1.1\r\nHost: " + (String)config.sensor_ip + "\r\nConnection: close\r\n\r\n";
            break;
        }

        //send the request
        if (client->space() > 32 && client->canSend()) {
            client->write(url.c_str());
            if (config.flags.moreDebug) { INFOV("Request sended to %s\n", IPAddress(client->getRemoteAddress()).toString().c_str()); }
        }
    });

    aClient->onData([](void *arg, AsyncClient *client, void *data, size_t len) {
        
        String chunk;

        chunk = String((char *)data);

        String i = midString(chunk, "Content-Length:", "\r\n");
        if (i != "")
        {
            message.totalMessageLength = i.toInt();
            message.headerLength = chunk.lastIndexOf("\r\n");
            message.messageLength = (int)len - message.headerLength - 2;
            message.message = chunk.substring(message.headerLength + 2, len);
        }
        else
        {
            message.messageLength += (int)len;
            message.message += chunk;
        }

        if (message.messageLength == message.totalMessageLength)
        {
            switch (config.wversion)
            {
                case 0: // Solax v2 local mode
                    parseJsonv2local(message.message);
                    break;
                case 1: // Solax v1
                    parseJsonv1(message.message);
                    break;
                case 2: // Solax v2
                    parseJson(message.message);
                    break;
                case 9: // Wibee
                    parseWibeee(message.message);
                    break;
                case 10: // Shelly EM
                    parseShellyEM(message.message, shellySensor);
                    shellySensor++;
                    if (shellySensor > 2)
                        shellySensor = 1;
                    break;
                case 12:
                    parseMasterFreeDs(message.message);
                    break;
                case 11: // Fronius
                    parseJson_fronius(message.message);
                    break;
            }
            client->close();
        }
    });

    if (config.wversion == 0) {
        if (!aClient->connect("5.8.8.8", 80))
        {
            if (config.flags.moreDebug) { INFOV("Connect Fail\n"); }
            AsyncClient *client = aClient;
            delete client;
            aClient = NULL;
        }
    } else {
        if (!aClient->connect(String(config.sensor_ip).c_str(), 80))
        {
            if (config.flags.moreDebug) { INFOV("Connect Fail\n"); }
            AsyncClient *client = aClient;
            delete client;
            aClient = NULL;
        }
    }
}