#include "RfidTag.h"

RfidTag::RfidTag(const byte *uid, const byte uidSize, const byte blockCount) {
    setUid(uid, uidSize);
    setBlockCount(blockCount);
}

bool RfidTag::operator==(const RfidTag &other) const {
    if (_uidSize != other._uidSize) {
        return false;
    }
    return memcmp(_uid, other._uid, _uidSize) == 0;
}

bool RfidTag::setUid(const byte *uid, const byte uidSize) {
    if (uid == nullptr) {
        return false;
    }
    if (uidSize != 4 && uidSize != 7 && uidSize != 10) {
        return false;
    }

    memcpy(_uid, uid, uidSize);
    if (uidSize < MAX_UID_SIZE) {
        memset(_uid + uidSize, 0, MAX_UID_SIZE - uidSize);
    }
    _uidSize = uidSize;
    return true;
}

bool RfidTag::getUid(byte *uidOut, byte &uidSizeOut) const {
    if (uidOut == nullptr) {
        return false;
    }

    uidSizeOut = _uidSize;
    memcpy(uidOut, _uid, _uidSize);
    return true;
}

const byte *RfidTag::getUid() const {
    return _uid;
}

byte RfidTag::getUidSize() const {
    return _uidSize;
}

bool RfidTag::setBlockCount(const byte blockCount) {
    if (blockCount > MAX_BLOCK_COUNT) {
        return false;
    }

    if (blockCount > _blockCount) {
        memset(_blockDataList[_blockCount], 0, static_cast<size_t>(blockCount - _blockCount) * BLOCK_SIZE);
    }
    _blockCount = blockCount;
    return true;
}

byte RfidTag::getBlockCount() const {
    return _blockCount;
}

bool RfidTag::setBlockData(const byte blockIndex, const byte *data16) {
    if (data16 == nullptr || blockIndex >= MAX_BLOCK_COUNT) {
        return false;
    }

    memcpy(_blockDataList[blockIndex], data16, BLOCK_SIZE);
    if (blockIndex >= _blockCount) {
        _blockCount = blockIndex + 1;
    }
    return true;
}

bool RfidTag::getBlockData(const byte blockIndex, byte *data16Out) const {
    if (data16Out == nullptr || blockIndex >= _blockCount) {
        return false;
    }

    memcpy(data16Out, _blockDataList[blockIndex], BLOCK_SIZE);
    return true;
}

bool RfidTag::setBlockDataList(const byte *flatData, const byte blockCount) {
    if (flatData == nullptr) {
        return false;
    }
    if (blockCount > MAX_BLOCK_COUNT) {
        return false;
    }

    if (blockCount > 0) {
        memcpy(_blockDataList[0], flatData, static_cast<size_t>(blockCount) * BLOCK_SIZE);
    }
    _blockCount = blockCount;
    return true;
}

bool RfidTag::getBlockDataList(byte *flatDataOut, const size_t outCapacity, size_t &outLen) const {
    outLen = static_cast<size_t>(_blockCount) * BLOCK_SIZE;
    if (flatDataOut == nullptr) {
        return false;
    }
    if (outCapacity < outLen) {
        return false;
    }
    if (outLen > 0) {
        memcpy(flatDataOut, _blockDataList[0], outLen);
    }
    return true;
}

void RfidTag::addBlockData(const byte block, byte data[16]) {
    setBlockData(block, data);
}

bool RfidTag::isHexSeparator(const char c) {
    return c == ' ' || c == ':' || c == '-' || c == '\t' || c == '\r' || c == '\n';
}

bool RfidTag::hexCharToNibble(const char c, byte &nibbleOut) {
    if (c >= '0' && c <= '9') {
        nibbleOut = static_cast<byte>(c - '0');
        return true;
    }
    if (c >= 'A' && c <= 'F') {
        nibbleOut = static_cast<byte>(10 + (c - 'A'));
        return true;
    }
    if (c >= 'a' && c <= 'f') {
        nibbleOut = static_cast<byte>(10 + (c - 'a'));
        return true;
    }
    return false;
}

char RfidTag::nibbleToHex(const byte nibble) {
    if (nibble < 10) {
        return static_cast<char>('0' + nibble);
    }
    return static_cast<char>('A' + (nibble - 10));
}

bool RfidTag::bytesToHex(
    const byte *input,
    const size_t inputLen,
    char *output,
    const size_t outputSize,
    const char separator
) {
    if (input == nullptr || output == nullptr) {
        return false;
    }

    const size_t separatorCount = (separator == '\0' || inputLen == 0) ? 0 : (inputLen - 1);
    const size_t required = (inputLen * 2) + separatorCount + 1;
    if (outputSize < required) {
        return false;
    }

    size_t outIdx = 0;
    for (size_t i = 0; i < inputLen; i++) {
        const byte value = input[i];
        output[outIdx++] = nibbleToHex(static_cast<byte>(value >> 4));
        output[outIdx++] = nibbleToHex(static_cast<byte>(value & 0x0F));
        if (separator != '\0' && i + 1 < inputLen) {
            output[outIdx++] = separator;
        }
    }

    output[outIdx] = '\0';
    return true;
}

bool RfidTag::hexToBytes(
    const char *input,
    byte *output,
    const size_t outputCapacity,
    size_t &outputLen
) {
    outputLen = 0;
    if (input == nullptr || output == nullptr) {
        return false;
    }

    bool hasHighNibble = false;
    byte highNibble = 0;

    for (size_t i = 0; input[i] != '\0'; i++) {
        const char c = input[i];
        if (isHexSeparator(c)) {
            continue;
        }

        byte nibble = 0;
        if (!hexCharToNibble(c, nibble)) {
            return false;
        }

        if (!hasHighNibble) {
            highNibble = nibble;
            hasHighNibble = true;
            continue;
        }

        if (outputLen >= outputCapacity) {
            return false;
        }

        output[outputLen++] = static_cast<byte>((highNibble << 4) | nibble);
        hasHighNibble = false;
    }

    return !hasHighNibble;
}

bool RfidTag::uidToHex(
    const byte *uid,
    const byte uidSize,
    char *output,
    const size_t outputSize,
    const char separator
) {
    if (uid == nullptr) {
        return false;
    }
    if (uidSize != 4 && uidSize != 7 && uidSize != 10) {
        return false;
    }
    return bytesToHex(uid, uidSize, output, outputSize, separator);
}

bool RfidTag::hexToUid(const char *input, byte *uid, byte &uidSize) {
    uidSize = 0;
    if (uid == nullptr) {
        return false;
    }

    size_t parsedLen = 0;
    if (!hexToBytes(input, uid, MAX_UID_SIZE, parsedLen)) {
        return false;
    }
    if (parsedLen != 4 && parsedLen != 7 && parsedLen != 10) {
        return false;
    }

    uidSize = static_cast<byte>(parsedLen);
    return true;
}

bool RfidTag::blockToHex(
    const byte *block16,
    char *output,
    const size_t outputSize,
    const char separator
) {
    if (block16 == nullptr) {
        return false;
    }
    return bytesToHex(block16, BLOCK_SIZE, output, outputSize, separator);
}

bool RfidTag::hexToBlock(const char *input, byte *block16) {
    if (block16 == nullptr) {
        return false;
    }

    size_t parsedLen = 0;
    if (!hexToBytes(input, block16, BLOCK_SIZE, parsedLen)) {
        return false;
    }
    return parsedLen == BLOCK_SIZE;
}

bool RfidTag::bytesToPrintableText(
    const byte *input,
    const size_t inputLen,
    char *output,
    const size_t outputSize,
    const char replacement
) {
    if (input == nullptr || output == nullptr) {
        return false;
    }
    if (outputSize < (inputLen + 1)) {
        return false;
    }

    for (size_t i = 0; i < inputLen; i++) {
        const byte value = input[i];
        if (value >= 32 && value <= 126) {
            output[i] = static_cast<char>(value);
        } else {
            output[i] = replacement;
        }
    }
    output[inputLen] = '\0';
    return true;
}

void RfidTag::textToBlock(const char *text, byte *block16, const byte fillByte) {
    if (block16 == nullptr) {
        return;
    }

    memset(block16, fillByte, BLOCK_SIZE);
    if (text == nullptr) {
        return;
    }

    for (byte i = 0; i < BLOCK_SIZE; i++) {
        if (text[i] == '\0') {
            return;
        }
        block16[i] = static_cast<byte>(text[i]);
    }
}
