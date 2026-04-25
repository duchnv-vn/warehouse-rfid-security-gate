#ifndef RFID_TAG_H
#define RFID_TAG_H

#include <Arduino.h>

class RfidTag {
    byte _uid[10] = {0};
    byte _uidSize = 0;
    byte _blockCount = 0;
    byte _blockDataList[64][16] = {{0}};

    static bool isHexSeparator(char c);

    static bool hexCharToNibble(char c, byte &nibbleOut);

    static char nibbleToHex(byte nibble);

public:
    static constexpr byte MAX_UID_SIZE = 10;
    static constexpr byte BLOCK_SIZE = 16;
    static constexpr byte MAX_BLOCK_COUNT = 64;

    RfidTag() = default;

    RfidTag(const byte *uid, byte uidSize, byte blockCount = 0);

    bool operator==(const RfidTag &other) const;

    bool setUid(const byte *uid, byte uidSize);

    bool getUid(byte *uidOut, byte &uidSizeOut) const;

    const byte *getUid() const;

    byte getUidSize() const;

    bool setBlockCount(byte blockCount);

    byte getBlockCount() const;

    bool setBlockData(byte blockIndex, const byte *data16);

    bool getBlockData(byte blockIndex, byte *data16Out) const;

    bool setBlockDataList(const byte *flatData, byte blockCount);

    bool getBlockDataList(byte *flatDataOut, size_t outCapacity, size_t &outLen) const;

    void addBlockData(byte block, byte data[16]);

    static bool bytesToHex(
        const byte *input,
        size_t inputLen,
        char *output,
        size_t outputSize,
        char separator = '\0'
    );

    static bool hexToBytes(
        const char *input,
        byte *output,
        size_t outputCapacity,
        size_t &outputLen
    );

    static bool uidToHex(
        const byte *uid,
        byte uidSize,
        char *output,
        size_t outputSize,
        char separator = ':'
    );

    static bool hexToUid(
        const char *input,
        byte *uid,
        byte &uidSize
    );

    static bool blockToHex(
        const byte *block16,
        char *output,
        size_t outputSize,
        char separator = ' '
    );

    static bool hexToBlock(
        const char *input,
        byte *block16
    );

    static bool bytesToPrintableText(
        const byte *input,
        size_t inputLen,
        char *output,
        size_t outputSize,
        char replacement = '.'
    );

    static void textToBlock(
        const char *text,
        byte *block16,
        byte fillByte = ' '
    );
};


#endif //RFID_TAG_H
