/* esp32ModbusTCP

Copyright 2020 Bert Melis

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "esp32ModbusTCP.h"

esp32ModbusTCP::esp32ModbusTCP(const uint8_t serverId, const IPAddress serverIP, const uint16_t port):
  _client(),
  _serverID(serverId),
  _serverIP(serverIP),
  _port(port),
  _state(DISCONNECTED),
  _lastMillis(0),
  _semaphore(nullptr),
  _toSend(),
  _toReceive(),
  _onDataHandler(nullptr),
  _onErrorHandler(nullptr),
  _currentResponse(nullptr) {
    _client.onConnect(_onConnect, this);
    _client.onDisconnect(_onDisconnect, this);
    _client.onError(_onError, this);
    _client.onTimeout(_onTimeout, this);
    _client.onPoll(_onPoll, this);
    _client.onData(_onData, this);
    _client.setNoDelay(true);
    _client.setAckTimeout(5000);
    _client.setRxTimeout(5000);
    _semaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(_semaphore);
}

esp32ModbusTCP::esp32ModbusTCP(const IPAddress serverIP, const uint16_t port):
  _client(),
  _serverID(0),
  _serverIP(serverIP),
  _port(port),
  _state(DISCONNECTED),
  _lastMillis(0),
  _semaphore(nullptr),
  _toSend(),
  _toReceive(),
  _onDataHandler(nullptr),
  _onErrorHandler(nullptr),
  _currentResponse(nullptr) {
    _client.onConnect(_onConnect, this);
    _client.onDisconnect(_onDisconnect, this);
    _client.onError(_onError, this);
    _client.onTimeout(_onTimeout, this);
    _client.onPoll(_onPoll, this);
    _client.onData(_onData, this);
    _client.setNoDelay(true);
    _client.setAckTimeout(5000);
    _client.setRxTimeout(5000);
    _semaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(_semaphore);
}

esp32ModbusTCP::~esp32ModbusTCP() {
  _toSend.clear();
  _toReceive.clear();
  vSemaphoreDelete(_semaphore);
}

void esp32ModbusTCP::onData(OnDataHandler handler) {
  _onDataHandler = handler;
}

void esp32ModbusTCP::onError(OnErrorHandler handler) {
  _onErrorHandler = handler;
}

bool esp32ModbusTCP::connect() {
  if (_state != DISCONNECTED) return false;

  log_v("connecting");
  if (xSemaphoreTake(_semaphore, 1000) == pdTRUE) {
    _state = CONNECTING;
    _client.connect(_serverIP, _port);
    xSemaphoreGive(_semaphore);
    return true;
  } else {
    log_e("couldn't obtain semaphore");
    return false;
  }
}

bool esp32ModbusTCP::disconnect(bool force) {
  log_v("disconnecting");
  if (xSemaphoreTake(_semaphore, 1000) == pdTRUE) {
    _state = DISCONNECTING;
    xSemaphoreGive(_semaphore);
    _clearQueue(esp32Modbus::COMM_ERROR);
    _client.close(force);
    return true;
  } else {
    log_e("couldn't obtain semaphore");
    return false;
  }
}

uint16_t esp32ModbusTCP::readHoldingRegisters(uint16_t address, uint16_t numberRegisters, void* arg) {
  ModbusRequest* request = new ModbusRequest03(_serverID, address, numberRegisters);
  if (request) return _addToQueue(request, arg);
  return 0;
}

uint16_t esp32ModbusTCP::writeHoldingRegister(uint16_t address, uint16_t data, void* arg) {
  ModbusRequest* request = new ModbusRequest06(_serverID, address, data);
  if (request) return _addToQueue(request, arg);
  return 0;
}

uint16_t esp32ModbusTCP::readHoldingRegisters(uint8_t serverId, uint16_t address, uint16_t numberRegisters, void* arg) {
  ModbusRequest* request = new ModbusRequest03(serverId, address, numberRegisters);
  if (request) return _addToQueue(request, arg);
  return 0;
}

uint16_t esp32ModbusTCP::writeHoldingRegister(uint8_t serverId, uint16_t address, uint16_t data, void* arg) {
  ModbusRequest* request = new ModbusRequest06(serverId, address, data);
  if (request) return _addToQueue(request, arg);
  return 0;
}

uint16_t esp32ModbusTCP::readInputRegisters(uint8_t serverId, uint16_t address, uint16_t numberRegisters, void* arg) {
  ModbusRequest* request = new ModbusRequest04(serverId, address, numberRegisters);
  if (request) return _addToQueue(request, arg);
  return 0;
}

uint16_t esp32ModbusTCP::_addToQueue(ModbusRequest* request, void* arg) {
  if (xSemaphoreTake(_semaphore, 1000) == pdTRUE) {
    if (_toSend.size() == MODBUS_MAX_QUEUE_SIZE) {
      delete request;
      xSemaphoreGive(_semaphore);
      return 0;
    }
    _toSend.emplace_back(request, arg);
    xSemaphoreGive(_semaphore);
    if (_state == DISCONNECTED) {
      connect();
    }
    return request->getId();
  } else {
    log_e("couldn't obtain semaphore");
    return 0;
  }
}

void esp32ModbusTCP::_tryToSend() {
  if (xSemaphoreTake(_semaphore, 1000) == pdTRUE) {
    if (_state == CONNECTED) {
      while (!_toSend.empty()) {
        ModbusAction& a = _toSend.front();
        if (_client.space() >= a.request->getSize()) {
          log_v("send id %d", a.request->getId());
          _client.add(reinterpret_cast<const char*>(a.request->getMessage()), a.request->getSize());
          _client.send();
          _toReceive.splice(_toReceive.end(), _toSend, _toSend.begin());
          _lastMillis = millis();
          delay(1);  // ensure we don't starve cpu
        } else {
          // stop looping
          break;
        }
      }
    }
    xSemaphoreGive(_semaphore);
  } else {
    log_e("couldn't obtain semaphore");
    return;
  }
}

void esp32ModbusTCP::_clearQueue(esp32Modbus::Error error) {
  if (xSemaphoreTake(_semaphore, 1000) == pdTRUE) {
    while(!_toSend.empty()) {
      //_tryError(_toSend.front().request->getId(), error, _toSend.front().arg);
      _toSend.pop_front();
    }
    while(!_toReceive.empty()) {
      //_tryError(_toSend.front().request->getId(), error, _toReceive.front().arg);
      _toReceive.pop_front();
    }
    xSemaphoreGive(_semaphore);
  } else {
    log_e("couldn't obtain semaphore");
    return;
  }
}

void esp32ModbusTCP::_tryError(uint16_t packetId, esp32Modbus::Error error, void* arg) {
  if (_onErrorHandler) _onErrorHandler(packetId, error, arg);
}

void esp32ModbusTCP::_tryData(ModbusResponse& response, void* arg) {
  if (_onDataHandler) {
    _onDataHandler(response.getId(),
                   response.getSlaveAddress(),
                   response.getFunctionCode(),
                   response.getData(),
                   response.getDataLength(),
                   arg);
  }
}

void esp32ModbusTCP::_onConnect(void* mb, AsyncClient* client) {
  log_v("connected");
  esp32ModbusTCP* c = reinterpret_cast<esp32ModbusTCP*>(mb);
  if (xSemaphoreTake(c->_semaphore, 1000) == pdTRUE) {
    c->_state = CONNECTED;
    xSemaphoreGive(c->_semaphore);
  } else {
    log_e("couldn't obtain semaphore");
    return;
  }

}

void esp32ModbusTCP::_onDisconnect(void* mb, AsyncClient* client) {
  log_v("disconnected");
  esp32ModbusTCP* c = reinterpret_cast<esp32ModbusTCP*>(mb);
  if (xSemaphoreTake(c->_semaphore, 1000) == pdTRUE) {
    c->_state = DISCONNECTED;
    if (!c->_toSend.empty() || !c->_toReceive.empty()) {
      xSemaphoreGive(c->_semaphore);
      c->connect();
    }
    xSemaphoreGive(c->_semaphore);
  } else {
    log_e("couldn't obtain semaphore");
    return;
  }

}

void esp32ModbusTCP::_onError(void* mb, AsyncClient* client, int8_t error) {
  log_v("TCP error");
  esp32ModbusTCP* c = reinterpret_cast<esp32ModbusTCP*>(mb);
  // Clearing queue to reset
  // _onDisconnected will be called afterwards
  c->_clearQueue(esp32Modbus::COMM_ERROR);
  c->disconnect();
}

void esp32ModbusTCP::_onTimeout(void* mb, AsyncClient* client, uint32_t time) {
  // clear queue and disconnect
  log_v("TCP timeout");
  esp32ModbusTCP* c = reinterpret_cast<esp32ModbusTCP*>(mb);
  c->_clearQueue(esp32Modbus::TIMEOUT);
  c->disconnect();
}

void esp32ModbusTCP::_onData(void* mb, AsyncClient* client, void* data, size_t length) {
  log_v("data: len %d", length);
  esp32ModbusTCP* c = reinterpret_cast<esp32ModbusTCP*>(mb);
  uint8_t* d = reinterpret_cast<uint8_t*>(data);
  size_t i = 0;
  if (xSemaphoreTake(c->_semaphore, 1000) != pdPASS) {
    log_e("couldn't obtain semaphore");
    return;
  }

  do {
    if (c->_currentResponse == nullptr) {
      if (length > 7) {  // create new Response object, need 7 bytes for MBAP header
        uint16_t packetLen = d[4] << 8 | d[5];
        c->_currentResponse = new ModbusResponse(packetLen);
      }
    } else if (c->_currentResponse) {
      c->_currentResponse->add(d[i++]);

      if (c->_currentResponse->isValid() && c->_currentResponse->isComplete()) {
        std::list<ModbusAction>::iterator it;
        for (it = c->_toReceive.begin(); it != (c->_toReceive.end()); ++it) {
          if (c->_currentResponse->match(*(it->request))) {
            if (c->_currentResponse->isSuccess()) {
              c->_tryData(*(c->_currentResponse), it->arg);
            } else {
              c->_tryError(c->_currentResponse->getId(), c->_currentResponse->getError(), it->arg);
            }
            c->_toReceive.erase(it);
            delete c->_currentResponse;
            c->_currentResponse = nullptr;
            break;
          }
        }
      }
    } else {
      log_e("Invalid packet received");
      _onError(c, nullptr, 0);
      delete c->_currentResponse;
    }
  } while (i < length);

  xSemaphoreGive(c->_semaphore);
}

void esp32ModbusTCP::_onPoll(void* mb, AsyncClient* client) {
  esp32ModbusTCP* c = reinterpret_cast<esp32ModbusTCP*>(mb);
  // try to send outing messages
  if (!(c->_toSend.empty())) {
    c->_tryToSend();
  } else if (c->_toReceive.empty() && millis() - c->_lastMillis > MODBUS_CONNECTION_MAX_IDLE_TIME) {
    c->disconnect();
  }
}
