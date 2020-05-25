/*

FAUXMO ESP

Copyright (C) 2016-2020 by Xose Pérez <xose dot perez at gmail dot com>

The MIT License (MIT)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#include <Arduino.h>
#include "fauxmoESP.h"

// -----------------------------------------------------------------------------
// UDP
// -----------------------------------------------------------------------------

void fauxmoESP::_sendUDPResponse() {

	DEBUG_MSG_FAUXMO("[FAUXMO] Responding to M-SEARCH request\n");

	IPAddress ip = WiFi.localIP();
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  mac.toLowerCase();

	char response[strlen(FAUXMO_UDP_RESPONSE_TEMPLATE) + 128];
  snprintf_P(response, sizeof(response), FAUXMO_UDP_RESPONSE_TEMPLATE, ip[0], ip[1], ip[2], ip[3], _tcp_port, mac.c_str(), mac.c_str());

	int len = strlen(response);
	_udp.beginPacket(_udp.remoteIP(), _udp.remotePort());
  _udp.write((uint8_t*)response, len);
  _udp.endPacket();

	#if DEBUG_FAUXMO_VERBOSE_UDP
    	DEBUG_MSG_FAUXMO("[FAUXMO] UDP response sent to %s:%d\n%s", _udp.remoteIP().toString().c_str(), _udp.remotePort(), response);
	#endif
}

void fauxmoESP::_handleUDP() {

	int len = _udp.parsePacket();
    if (len > 0) {

		unsigned char data[len+1];
        _udp.read(data, len);
        data[len] = 0;

		#if DEBUG_FAUXMO_VERBOSE_UDP
			DEBUG_MSG_FAUXMO("[FAUXMO] UDP packet received\n%s", (const char *) data);
		#endif

        String request = (const char *) data;
        if (request.indexOf("M-SEARCH") >= 0) {
			if ((request.indexOf("upnp:rootdevice") > 0) || (request.indexOf("device:basic:1") > 0) || (request.indexOf("ssdp:all") > 0)) {
                _sendUDPResponse();
            }
        }
    }

}


// -----------------------------------------------------------------------------
// TCP
// -----------------------------------------------------------------------------

void fauxmoESP::_sendTCPResponse(AsyncClient *client, const char * code, char * body, const char * mime) {

	char headers[strlen_P(FAUXMO_TCP_HEADERS) + 32];
	snprintf_P(
		headers, sizeof(headers),
		FAUXMO_TCP_HEADERS,
		code, mime, strlen(body)
	);

	#if DEBUG_FAUXMO_VERBOSE_TCP
		DEBUG_MSG_FAUXMO("[FAUXMO] Sending TCP Response:\n%s%s\n", headers, body);
	#endif

	client->write(headers);
	client->write(body);
}

String fauxmoESP::_deviceJson(unsigned char id) {

	if (id >= _devices.size()) return "{}";

	String mac = WiFi.macAddress();
    mac.replace(":", "");
    mac.toLowerCase();

	fauxmoesp_device_t device = _devices[id];
	if (device.value == 0) device.value++;
  char buffer[strlen_P(FAUXMO_DEVICE_JSON_TEMPLATE_DIMMABLE) + 64];
  if (device.type == ONOFF) {
    snprintf_P(
        buffer, sizeof(buffer),
        FAUXMO_DEVICE_JSON_TEMPLATE_ONOFF,
        device.name, mac.c_str(), id+1,
        device.state ? "true": "false"
    );
    #if DEBUG_FAUXMO_VERBOSE_TCP
		  DEBUG_MSG_FAUXMO("[FAUXMO] MODO ON/OFF %s\n", device.name);
	  #endif
  } else {
    snprintf_P(
      buffer, sizeof(buffer),
      FAUXMO_DEVICE_JSON_TEMPLATE_DIMMABLE,
      device.name, mac.c_str(), id+1,
      device.state ? "true": "false",
      device.value
    );
    #if DEBUG_FAUXMO_VERBOSE_TCP
		  DEBUG_MSG_FAUXMO("[FAUXMO] MODO DIMMABLE %s\n", device.name);
	  #endif
  }

	return String(buffer);
}

bool fauxmoESP::_onTCPDescription(AsyncClient *client, String url, String body) {

	(void) url;
	(void) body;

	DEBUG_MSG_FAUXMO("[FAUXMO] Handling /description.xml request\n");

	IPAddress ip = WiFi.localIP();
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    mac.toLowerCase();

	char response[strlen_P(FAUXMO_DESCRIPTION_TEMPLATE) + 64];
    snprintf_P(
        response, sizeof(response),
        FAUXMO_DESCRIPTION_TEMPLATE,
        ip[0], ip[1], ip[2], ip[3], _tcp_port,
        ip[0], ip[1], ip[2], ip[3], _tcp_port,
        mac.c_str(), mac.c_str()
    );

	_sendTCPResponse(client, "200 OK", response, "text/xml");

	return true;
}

bool fauxmoESP::_onTCPList(AsyncClient *client, String url, String body) {

	DEBUG_MSG_FAUXMO("[FAUXMO] Handling list request\n");

	// Get the index
	int pos = url.indexOf("lights");
	if (-1 == pos) return false;

	// Get the id
	unsigned char id = url.substring(pos+7).toInt();

	// This will hold the response string	
	String response;

	// Client is requesting all devices
	if (0 == id) {

		response += "{";
		for (unsigned char i=0; i< _devices.size(); i++) {
			if (i>0) response += ",";
			response += "\"" + String(i+1) + "\":" + _deviceJson(i);
		}
		response += "}";

	// Client is requesting a single device
	} else {
		response = _deviceJson(id-1);
	}

	_sendTCPResponse(client, "200 OK", (char *) response.c_str(), "application/json");
	
	return true;
}

bool fauxmoESP::_onTCPControl(AsyncClient *client, String url, String body) {

	#if DEBUG_FAUXMO_VERBOSE_TCP
        DEBUG_MSG_FAUXMO("[FAUXMO] _onTCPControl url: %s\n", url.c_str());
        DEBUG_MSG_FAUXMO("[FAUXMO] _onTCPControl body: %s\n", body.c_str());
	#endif
	
	// "devicetype" request
	if (body.indexOf("devicetype") > 0) {
		DEBUG_MSG_FAUXMO("[FAUXMO] Handling devicetype request\n");
		_sendTCPResponse(client, "200 OK", (char *) "[{\"success\":{\"username\": \"2WLEDHardQrI3WHYTHoMcXHgEspsM8ZZRpSKtBQr\"}}]", "application/json");
		return true;
	}

	// "state" request
	if ((url.indexOf("state") > 0) && (body.length() > 0)) {
		_sendTCPResponse(client, "200 OK", (char *) "[{\"success\":{\"/lights/1/state/\": true}}]", "application/json");
		
		DEBUG_MSG_FAUXMO("[FAUXMO] Handling state request\n");

		// Get the index
		unsigned char id = url.substring(url.indexOf("lights")+7).toInt();
		if (id > 0) {

			--id;

			// Brightness
			int pos = body.indexOf("bri");
			if (pos > 0) {
				unsigned char value = body.substring(pos+5).toInt();
				_devices[id].value = value;
				_devices[id].state = (value > 0);
			} else if (body.indexOf("false") > 0) {
				_devices[id].state = false;
			} else {
				_devices[id].state = true;
				if (0 == _devices[id].value) _devices[id].value = 254;
			}

			if (_setCallback) {
				_setCallback(id, _devices[id].name, _devices[id].state, _devices[id].value);
			}

			return true;
		}

	}

	// "lights" request
	if (url.indexOf("lights") > 0) {

		DEBUG_MSG_FAUXMO("[FAUXMO] Handling lights state\n");

		// Get the index
		unsigned char id = url.substring(url.indexOf("lights")+7).toInt();
		if (id > 0) {
			_sendTCPResponse(client, "200 OK", (char *) _deviceJson(id - 1).c_str(), "application/json");
			return true;
		}

	}

	return false;
}

bool fauxmoESP::_onTCPRequest(AsyncClient *client, bool isGet, String url, String body) {

  if (!_enabled) return false;

	#if DEBUG_FAUXMO_VERBOSE_TCP
		DEBUG_MSG_FAUXMO("[FAUXMO] isGet: %s\n", isGet ? "true" : "false");
		DEBUG_MSG_FAUXMO("[FAUXMO] URL: %s\n", url.c_str());
		if (!isGet) DEBUG_MSG_FAUXMO("[FAUXMO] Body:\n%s\n", body.c_str());
	#endif

	if (url.equals("/description.xml")) {
      return _onTCPDescription(client, url, body);
    }

	if (url.startsWith("/api")) {
		if (isGet) {
			return _onTCPList(client, url, body);
		} else {
      return _onTCPControl(client, url, body);
		}
	}

	return false;
}


// -----------------------------------------------------------------------------
// Devices
// -----------------------------------------------------------------------------

fauxmoESP::~fauxmoESP() {
  	
	// Free the name for each device
	for (auto& device : _devices) {
		free(device.name);
  	}
  	
	// Delete devices  
	_devices.clear();

}

unsigned char fauxmoESP::addDevice(const char * device_name, uint8_t type) {

  fauxmoesp_device_t device;
  unsigned int device_id = _devices.size();

  // init properties
  device.name = strdup(device_name);
  device.type = type;
  device.state = false;
  device.value = 0;

  // Attach
  _devices.push_back(device);

  Serial.printf("[FAUXMO] Device '%s' added as #%d\n", device_name, device_id);

  return device_id;
}

int fauxmoESP::getDeviceId(const char * device_name) {
    for (unsigned int id=0; id < _devices.size(); id++) {
        if (strcmp(_devices[id].name, device_name) == 0) {
            return id;
        }
    }
    return -1;
}

bool fauxmoESP::renameDevice(unsigned char id, const char * device_name) {
    if (id < _devices.size()) {
        free(_devices[id].name);
        _devices[id].name = strdup(device_name);
        Serial.printf("[FAUXMO] Device #%d renamed to '%s'\n", id, device_name);
        return true;
    }
    return false;
}

bool fauxmoESP::renameDevice(const char * old_device_name, const char * new_device_name) {
	int id = getDeviceId(old_device_name);
	if (id < 0) return false;
	return renameDevice(id, new_device_name);
}

bool fauxmoESP::removeDevice(unsigned char id) {
    if (id < _devices.size()) {
        free(_devices[id].name);
	    	_devices.erase(_devices.begin()+id);
        Serial.printf("[FAUXMO] Device #%d removed\n", id);
        return true;
    }
    return false;
}

bool fauxmoESP::removeDevice(const char * device_name) {
	int id = getDeviceId(device_name);
	if (id < 0) return false;
	return removeDevice(id);
}

char * fauxmoESP::getDeviceName(unsigned char id, char * device_name, size_t len) {
    if ((id < _devices.size()) && (device_name != NULL)) {
        strncpy(device_name, _devices[id].name, len);
    }
    return device_name;
}

bool fauxmoESP::setState(unsigned char id, bool state, unsigned char value) {
    if (id < _devices.size()) {
		_devices[id].state = state;
		_devices[id].value = value;
		return true;
	}
	return false;
}

bool fauxmoESP::setState(const char * device_name, bool state, unsigned char value) {
	int id = getDeviceId(device_name);
	if (id < 0) return false;
	_devices[id].state = state;
	_devices[id].value = value;
	return true;
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

bool fauxmoESP::process(AsyncClient *client, bool isGet, String url, String body) {
	return _onTCPRequest(client, isGet, url, body);
}

void fauxmoESP::handle() {
    if (_enabled) _handleUDP();
}

void fauxmoESP::enable(bool enable) {

	if (enable == _enabled) return;
  
  _enabled = enable;

	if (_enabled) {
		Serial.printf("[FAUXMO] Enabled\n");
	} else {
		Serial.printf("[FAUXMO] Disabled\n");
	}

  if (_enabled) {
    // UDP setup
    _udp.beginMulticast(FAUXMO_UDP_MULTICAST_IP, FAUXMO_UDP_MULTICAST_PORT);
    Serial.printf("[FAUXMO] UDP server started\n");
	} else {
    _udp.stop();
    Serial.printf("[FAUXMO] UDP server stopped\n");
  }

}
