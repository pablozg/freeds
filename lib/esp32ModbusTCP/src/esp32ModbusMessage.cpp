/* esp32ModbusMessage

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

#include <Arduino.h>

#include "esp32ModbusMessage.h"

uint8_t low(uint16_t in) {
  return (in & 0xff);
}

uint8_t high(uint16_t in) {
  return ((in >> 8) & 0xff);
}

uint16_t make_word(uint8_t high, uint8_t low) {
  return ((high << 8) | low);
}

bool getBit(uint8_t byte, uint8_t pos) {
  return (byte >> pos) & 0x01;
}

void setBit(uint8_t byte, uint8_t pos, bool bit) {
  uint8_t mask;
  if (bit) {
    mask = 0x00;
    setBit(mask, pos, bit);
    byte |= mask;
  } else {
    mask = 0xFF;
    setBit(mask, pos, bit);
    byte &= mask;
  }
}

ModbusMessage::~ModbusMessage() {
  delete[] _buffer;
}

void ModbusMessage::add(uint8_t data) {
  if (_index < _totalLength) _buffer[_index++] = data;
}

uint8_t* ModbusMessage::getMessage() const {
  return &_buffer[0];
}

size_t ModbusMessage::getSize() const {
  return _index;  // _index is already incremented
}

bool ModbusMessage::isComplete() const {
  if (_index == _totalLength) return true;
  return false;
}

ModbusMessage::ModbusMessage(size_t totalLength) :
  _buffer(nullptr),
  _totalLength(totalLength),
  _index(0) {
    if (_totalLength > 256) _totalLength = 256;  // max Modbus length message
    _buffer = new uint8_t[_totalLength];
  }

uint16_t ModbusRequest::_lastPacketId = 0;

uint16_t ModbusRequest::getId() const {
  return _packetId;
}

ModbusRequest::ModbusRequest(size_t totalLength) :
  ModbusMessage(totalLength),
  _packetId(0),
  _slaveAddress(0),
  _functionCode(0),
  _address(0),
  _byteCount(0) {
    _packetId = ++_lastPacketId;
    if (_lastPacketId == 0) _lastPacketId = 1;
  }

ModbusRequest02::ModbusRequest02(uint8_t slaveAddress, uint16_t address, uint16_t numberCoils) :
  ModbusRequest(12) {
  _slaveAddress = slaveAddress;
  _functionCode = esp32Modbus::READ_DISCR_INPUT;
  _address = address;
  _byteCount = numberCoils / 8 + 1;
  add(high(_packetId));
  add(low(_packetId));
  add(0x00);
  add(0x00);
  add(0x00);
  add(0x06);
  add(_slaveAddress);
  add(_functionCode);
  add(high(_address));
  add(low(_address));
  add(high(numberCoils));
  add(low(numberCoils));
}

size_t ModbusRequest02::responseLength() const {
  return 9 + _byteCount;
}

ModbusRequest03::ModbusRequest03(uint8_t slaveAddress, uint16_t address, uint16_t numberRegisters) :
  ModbusRequest(12) {
  _slaveAddress = slaveAddress;
  _functionCode = esp32Modbus::READ_HOLD_REGISTER;
  _address = address;
  _byteCount = numberRegisters * 2;  // register is 2 bytes wide
  add(high(_packetId));
  add(low(_packetId));
  add(0x00);
  add(0x00);
  add(0x00);
  add(0x06);
  add(_slaveAddress);
  add(_functionCode);
  add(high(_address));
  add(low(_address));
  add(high(numberRegisters));
  add(low(numberRegisters));
}

size_t ModbusRequest03::responseLength() const {
  return 9 + _byteCount;
}

ModbusRequest04::ModbusRequest04(uint8_t slaveAddress, uint16_t address, uint16_t numberRegisters) :
  ModbusRequest(12) {
  _slaveAddress = slaveAddress;
  _functionCode = esp32Modbus::READ_INPUT_REGISTER;
  _address = address;
  _byteCount = numberRegisters * 2;  // register is 2 bytes wide
  add(high(_packetId));
  add(low(_packetId));
  add(0x00);
  add(0x00);
  add(0x00);
  add(0x06);
  add(_slaveAddress);
  add(_functionCode);
  add(high(_address));
  add(low(_address));
  add(high(numberRegisters));
  add(low(numberRegisters));
}

size_t ModbusRequest04::responseLength() const {
  return 9 + _byteCount;
}

ModbusRequest06::ModbusRequest06(uint8_t slaveAddress, uint16_t address, uint16_t data) :
  ModbusRequest(12) {
  _slaveAddress = slaveAddress;
  _functionCode = esp32Modbus::WRITE_HOLD_REGISTER;
  _address = address;
  _byteCount = 2;  // register is 2 bytes wide
  add(high(_packetId));
  add(low(_packetId));
  add(0x00);
  add(0x00);
  add(0x00);
  add(0x06);
  add(_slaveAddress);
  add(_functionCode);
  add(high(_address));
  add(low(_address));
  add(high(data));
  add(low(data));
}

size_t ModbusRequest06::responseLength() const {
  return 12;
}

ModbusResponse::ModbusResponse(uint16_t dataLength) :
  ModbusMessage(6 + dataLength),
  _dataLength(dataLength),
  _error(esp32Modbus::SUCCESS) {
  }

bool ModbusResponse::isValid() const {
  if (_index < 7) return false;  // MBAP header length
  if (_buffer[2] != 0 || _buffer[3] != 0) return false;  // protocol ID
  if (_dataLength != make_word(_buffer[4], _buffer[5])) return false;
  if (_dataLength > 256) return false;
  else return true;
}

bool ModbusResponse::match(const ModbusRequest& request) const {
  if (request.getId() != make_word(_buffer[0], _buffer[1])) return false;
  // add extra checks: address, number registers... (possible error response!)
  return true;
}

bool ModbusResponse::isSuccess() const {
  if (_buffer[7] > 0x80) return false;
  return true;
}

esp32Modbus::Error ModbusResponse::getError() const {
  return static_cast<esp32Modbus::Error>(_buffer[8]);
}

uint16_t ModbusResponse::getId() const {
  return make_word(_buffer[0], _buffer[1]);
}

uint8_t ModbusResponse::getSlaveAddress() const {
  return _buffer[6];
}

esp32Modbus::FunctionCode ModbusResponse::getFunctionCode() const {
  return static_cast<esp32Modbus::FunctionCode>(_buffer[7]);
}

uint8_t* ModbusResponse::getData() const {
  return &_buffer[9];
}

size_t ModbusResponse::getDataLength() const {
  return _dataLength;
}
