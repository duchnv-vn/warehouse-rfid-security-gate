#include "InterBoardSerialBus.h"

InterBoardSerialBus::InterBoardSerialBus(Stream &serialPort)
    : _serialPort(serialPort),
      _handlerCount(0),
      _unknownHandler(nullptr),
      _rxState(RX_WAIT_START),
      _rxType(0),
      _rxLength(0),
      _rxPayload{0},
      _rxIndex(0),
      _rxChecksum(0) {
    for (byte i = 0; i < MAX_HANDLER_COUNT; i++) {
        _handlers[i].type = 0;
        _handlers[i].handler = nullptr;
    }
}

void InterBoardSerialBus::on(const byte type, MessageHandler handler) {
    for (byte i = 0; i < _handlerCount; i++) {
        if (_handlers[i].type == type) {
            _handlers[i].handler = handler;
            return;
        }
    }

    if (_handlerCount >= MAX_HANDLER_COUNT) {
        return;
    }

    _handlers[_handlerCount].type = type;
    _handlers[_handlerCount].handler = handler;
    _handlerCount++;
}

void InterBoardSerialBus::onUnknown(MessageHandler handler) {
    _unknownHandler = handler;
}

bool InterBoardSerialBus::send(const byte type, const byte *payload, const byte payloadLength) {
    if (payloadLength > MAX_PAYLOAD_SIZE) {
        return false;
    }
    if (payloadLength > 0 && payload == nullptr) {
        return false;
    }

    const byte checksum = calculateChecksum(type, payloadLength, payload);

    if (_serialPort.write(START_BYTE) != 1) {
        return false;
    }
    if (_serialPort.write(type) != 1) {
        return false;
    }
    if (_serialPort.write(payloadLength) != 1) {
        return false;
    }
    if (payloadLength > 0 && _serialPort.write(payload, payloadLength) != payloadLength) {
        return false;
    }
    return _serialPort.write(checksum) == 1;
}

bool InterBoardSerialBus::sendText(const byte type, const char *text) {
    if (text == nullptr) {
        return false;
    }

    const size_t textLength = strlen(text);
    if (textLength > MAX_PAYLOAD_SIZE) {
        return false;
    }
    return send(type, reinterpret_cast<const byte *>(text), static_cast<byte>(textLength));
}

bool InterBoardSerialBus::sendByte(const byte type, const byte value) {
    return send(type, &value, 1);
}

bool InterBoardSerialBus::sendUInt16(const byte type, const uint16_t value) {
    byte payload[2] = {0};
    payload[0] = static_cast<byte>((value >> 8) & 0xFF);
    payload[1] = static_cast<byte>(value & 0xFF);
    return send(type, payload, sizeof(payload));
}

bool InterBoardSerialBus::sendNewDirection(const byte directionCode) {
    return sendByte(MSG_NEW_DIRECTION, directionCode);
}

void InterBoardSerialBus::poll() {
    while (_serialPort.available() > 0) {
        const int readValue = _serialPort.read();
        if (readValue < 0) {
            break;
        }

        const byte input = static_cast<byte>(readValue);

        switch (_rxState) {
            case RX_WAIT_START:
                if (input == START_BYTE) {
                    _rxState = RX_READ_TYPE;
                }
                break;

            case RX_READ_TYPE:
                _rxType = input;
                _rxChecksum = input;
                _rxState = RX_READ_LENGTH;
                break;

            case RX_READ_LENGTH:
                _rxLength = input;
                _rxChecksum ^= input;
                _rxIndex = 0;

                if (_rxLength > MAX_PAYLOAD_SIZE) {
                    resetRx();
                    break;
                }

                _rxState = (_rxLength == 0) ? RX_READ_CHECKSUM : RX_READ_PAYLOAD;
                break;

            case RX_READ_PAYLOAD:
                _rxPayload[_rxIndex++] = input;
                _rxChecksum ^= input;
                if (_rxIndex >= _rxLength) {
                    _rxState = RX_READ_CHECKSUM;
                }
                break;

            case RX_READ_CHECKSUM:
                if (_rxChecksum == input) {
                    dispatch(_rxType, _rxPayload, _rxLength);
                }
                resetRx();
                break;

            default:
                resetRx();
                break;
        }
    }
}

byte InterBoardSerialBus::calculateChecksum(const byte type, const byte payloadLength, const byte *payload) {
    byte checksum = static_cast<byte>(type ^ payloadLength);
    for (byte i = 0; i < payloadLength; i++) {
        checksum ^= payload[i];
    }
    return checksum;
}

void InterBoardSerialBus::resetRx() {
    _rxState = RX_WAIT_START;
    _rxType = 0;
    _rxLength = 0;
    _rxIndex = 0;
    _rxChecksum = 0;
}

void InterBoardSerialBus::dispatch(const byte type, const byte *payload, const byte payloadLength) const {
    for (byte i = 0; i < _handlerCount; i++) {
        if (_handlers[i].type == type && _handlers[i].handler != nullptr) {
            _handlers[i].handler(type, payload, payloadLength);
            return;
        }
    }

    if (_unknownHandler != nullptr) {
        _unknownHandler(type, payload, payloadLength);
    }
}

bool InterBoardSerialBus::sendNewTag(const RfidTag &tag) {
    byte uid[RfidTag::MAX_UID_SIZE] = {0};
    byte uidSize = 0;
    if (!tag.getUid(uid, uidSize)) {
        return false;
    }
    if (uidSize != 4 && uidSize != 7 && uidSize != 10) {
        return false;
    }

    byte payload[1 + RfidTag::MAX_UID_SIZE] = {0};
    payload[0] = uidSize;
    memcpy(payload + 1, uid, uidSize);

    return send(MSG_NEW_TAG, payload, static_cast<byte>(1 + uidSize));
}

bool InterBoardSerialBus::parseNewTag(
    const byte *payload,
    const byte payloadLength,
    TagUid &tagOut
) {
    tagOut.uidSize = 0;
    memset(tagOut.uid, 0, sizeof(tagOut.uid));

    if (payload == nullptr || payloadLength < 2) {
        return false;
    }

    const byte uidSize = payload[0];
    if (uidSize != 4 && uidSize != 7 && uidSize != 10) {
        return false;
    }
    if (payloadLength != static_cast<byte>(1 + uidSize)) {
        return false;
    }

    tagOut.uidSize = uidSize;
    memcpy(tagOut.uid, payload + 1, uidSize);
    return true;
}

bool InterBoardSerialBus::parseNewDirection(
    const byte *payload,
    const byte payloadLength,
    byte &directionCodeOut
) {
    directionCodeOut = DIR_UNKNOWN;
    if (payload == nullptr || payloadLength != 1) {
        return false;
    }

    const byte directionCode = payload[0];
    if (directionCode != DIR_OUT_IN && directionCode != DIR_IN_OUT && directionCode != DIR_UNKNOWN) {
        return false;
    }

    directionCodeOut = directionCode;
    return true;
}

bool InterBoardSerialBus::sendNewScanningStatus(const byte scanning) {
    return sendByte(NSG_NEW_SCAN_STATUS, scanning);
}

bool InterBoardSerialBus::parseNewScanningStatus(const byte *payload, byte payloadLength, byte &scanningStatusOut) {
    scanningStatusOut = SCAN_UNKNOWN;

    if (payload == nullptr || payloadLength != 1) {
        return false;
    }

    const byte scanningStatus = payload[0];

    if (scanningStatus != START_SCAN && scanningStatus != STOP_SCAN) {
        return false;
    }

    scanningStatusOut = scanningStatus;
    return true;
}
