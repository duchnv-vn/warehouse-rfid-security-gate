#ifndef INTER_BOARD_SERIAL_BUS_H
#define INTER_BOARD_SERIAL_BUS_H

#include <Arduino.h>
#include "RfidTag.h"

class InterBoardSerialBus {
public:
    using MessageHandler = void (*)(byte type, const byte *payload, byte payloadLength);

    // Packet format: [START][TYPE][LEN][PAYLOAD...][CHECKSUM]
    // CHECKSUM is XOR(TYPE, LEN, PAYLOAD bytes).
    static constexpr byte START_BYTE = 0xAA;
    // Fits 1 direction + 1 tagCount + (10 tags * (1 uidSize + 10 uid bytes)) = 112 bytes
    static constexpr byte MAX_PAYLOAD_SIZE = 120;
    static constexpr byte MAX_TAGS_PER_NEW_EVENT = 10;
    static constexpr byte MAX_HANDLER_COUNT = 8;

    enum MessageType : byte {
        MSG_HEARTBEAT = 1,
        MSG_NEW_TAG = 2,
        MSG_NEW_DIRECTION = 3,
        NSG_NEW_SCAN_STATUS = 4,
        MSG_ACK = 11,
        MSG_ERROR = 12
    };

    enum DirectionCode : byte {
        DIR_OUT_IN = 0,
        DIR_IN_OUT = 1,
        DIR_UNKNOWN = 2
    };

    enum ScanningStatusCode : byte {
        STOP_SCAN = 0,
        START_SCAN = 1,
        SCAN_UNKNOWN = 2
    };

    struct TagUid {
        byte uidSize;
        byte uid[RfidTag::MAX_UID_SIZE];
    };

    explicit InterBoardSerialBus(Stream &serialPort);

    void on(byte type, MessageHandler handler);

    void onUnknown(MessageHandler handler);

    bool send(byte type, const byte *payload, byte payloadLength);

    bool sendText(byte type, const char *text);

    bool sendByte(byte type, byte value);

    bool sendUInt16(byte type, uint16_t value);

    // NEW_DIRECTION payload format: [directionCode(1)]
    bool sendNewDirection(byte directionCode);

    bool sendNewTag(const RfidTag &tag);

    bool sendNewScanningStatus(byte scanning);

    // NEW_TAG payload format: [uidSize(1)][uid(uidSize)]
    static bool parseNewTag(
        const byte *payload,
        byte payloadLength,
        TagUid &tagOut
    );

    static bool parseNewDirection(
        const byte *payload,
        byte payloadLength,
        byte &directionCodeOut
    );

    static bool parseNewScanningStatus(
        const byte *payload,
        byte payloadLength,
        byte &scanningstatusOut
    );

    // Call repeatedly in loop() to parse incoming packets and dispatch handlers.
    void poll();

private:
    enum RxState : byte {
        RX_WAIT_START = 0,
        RX_READ_TYPE = 1,
        RX_READ_LENGTH = 2,
        RX_READ_PAYLOAD = 3,
        RX_READ_CHECKSUM = 4
    };

    struct HandlerEntry {
        byte type;
        MessageHandler handler;
    };

    Stream &_serialPort;
    HandlerEntry _handlers[MAX_HANDLER_COUNT];
    byte _handlerCount;
    MessageHandler _unknownHandler;

    RxState _rxState;
    byte _rxType;
    byte _rxLength;
    byte _rxPayload[MAX_PAYLOAD_SIZE];
    byte _rxIndex;
    byte _rxChecksum;

    static byte calculateChecksum(byte type, byte payloadLength, const byte *payload);

    void resetRx();

    void dispatch(byte type, const byte *payload, byte payloadLength) const;
};

#endif // INTER_BOARD_SERIAL_BUS_H
