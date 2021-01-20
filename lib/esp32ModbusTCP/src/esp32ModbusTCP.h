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

#pragma once

#include <functional>
#include <list>

#include <FreeRTOS.h>  // must appear before semphr.h
#include <freertos/semphr.h>

#include <esp32-hal.h>  // for millis() and logging

#include <AsyncTCP.h>  // also includes IPAddress.h

#include "esp32ModbusTypeDefs.h"
#include "esp32ModbusMessage.h"

#ifndef MODBUS_MAX_QUEUE_SIZE
#define MODBUS_MAX_QUEUE_SIZE 20
#endif
#ifndef MODBUS_CONNECTION_MAX_IDLE_TIME
#define MODBUS_CONNECTION_MAX_IDLE_TIME 10000
#endif

class esp32ModbusTCP {

  typedef std::function<void(uint16_t packetId, uint8_t slaveAddress, esp32Modbus::FunctionCode fc, uint8_t* data, uint16_t len, void* arg)> OnDataHandler;
  typedef std::function<void(uint16_t packetId, esp32Modbus::Error error, void* arg)> OnErrorHandler;

  struct ModbusAction {
    ModbusAction(ModbusRequest* req, void* a):
      request(req),
      arg(a) {}
    ~ModbusAction() {
      delete request;
    }
    ModbusRequest* request;
    void* arg;
  };

 public:
  esp32ModbusTCP(const uint8_t serverId, const IPAddress serverIP, const uint16_t port = 502);
  explicit esp32ModbusTCP(const IPAddress serverIP, const uint16_t port = 502);
  ~esp32ModbusTCP();
  void onData(OnDataHandler handler);
  void onError(OnErrorHandler handler);
  bool connect();
  bool disconnect(bool force = false);
  uint16_t readHoldingRegisters(uint16_t address, uint16_t numberRegisters, void* arg = nullptr);
  uint16_t writeHoldingRegister(uint16_t address, uint16_t data, void* arg = nullptr);
  uint16_t readHoldingRegisters(uint8_t serverId, uint16_t address, uint16_t numberRegisters, void* arg = nullptr);
  uint16_t writeHoldingRegister(uint8_t serverId, uint16_t address, uint16_t data, void* arg = nullptr);
  uint16_t readInputRegisters(uint8_t serverId, uint16_t address, uint16_t numberRegisters, void* arg = nullptr);

 private:
  uint16_t _addToQueue(ModbusRequest* request, void* arg);
  void _tryToSend();
  void _clearQueue(esp32Modbus::Error error);
  void _tryError(uint16_t packetId, esp32Modbus::Error error, void* arg);
  void _tryData(ModbusResponse& response, void* arg);
  void _connect();
  void _disconnect(bool now = false);
  static void _onConnect(void* mb, AsyncClient* client);
  static void _onDisconnect(void* mb, AsyncClient* client);
  static void _onError(void* mb, AsyncClient* client, int8_t error);
  static void _onTimeout(void* mb, AsyncClient* client, uint32_t time);
  static void _onData(void* mb, AsyncClient* client, void* data, size_t length);
  static void _onPoll(void* mb, AsyncClient* client);

  AsyncClient _client;
  const uint8_t _serverID;
  const IPAddress _serverIP;
  const uint16_t _port;
  enum {
    DISCONNECTED,
    CONNECTING,
    DISCONNECTING,
    CONNECTED
  } _state;
  uint32_t _lastMillis;
  SemaphoreHandle_t _semaphore;
  std::list<ModbusAction> _toSend;
  std::list<ModbusAction> _toReceive;
  OnDataHandler _onDataHandler;
  OnErrorHandler _onErrorHandler;
  ModbusResponse* _currentResponse;
};
