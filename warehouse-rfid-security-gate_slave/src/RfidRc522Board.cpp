#include "RfidRc522Board.h"

bool RfidRc522Board::isNewCardPresent() {
    return rfid.PICC_IsNewCardPresent();
}

bool RfidRc522Board::readCardSerial() {
    return rfid.PICC_ReadCardSerial();
}

MFRC522::Uid RfidRc522Board::getUID() const {
    return rfid.uid;
}

void RfidRc522Board::reset() {
    rfid.PCD_Reset();
}

bool RfidRc522Board::isSectorTrailerBlock(const byte blockAddress) {
    if (blockAddress < 128) {
        return ((blockAddress + 1) % 4) == 0;
    }
    return ((blockAddress + 1) % 16) == 0;
}

bool RfidRc522Board::isUserDataBlock(const byte blockAddress) {
    if (blockAddress == 0) {
        return false;
    }
    return !isSectorTrailerBlock(blockAddress);
}

void RfidRc522Board::setDefaultMifareKey(MFRC522::MIFARE_Key &key) {
    for (byte i = 0; i < MFRC522::MF_KEY_SIZE; i++) {
        key.keyByte[i] = 0xFF;
    }
}

bool RfidRc522Board::ensureCardSelected() {
    const bool isCardPresent = rfid.PICC_IsNewCardPresent();
    if (!isCardPresent) {
        return false;
    }

    const bool readCardSerial = rfid.PICC_ReadCardSerial();
    return readCardSerial;
}

bool RfidRc522Board::isMifareClassicCard() const {
    const MFRC522::PICC_Type cardType = rfid.PICC_GetType(rfid.uid.sak);
    return cardType == MFRC522::PICC_TYPE_MIFARE_MINI
           || cardType == MFRC522::PICC_TYPE_MIFARE_1K
           || cardType == MFRC522::PICC_TYPE_MIFARE_4K;
}

bool RfidRc522Board::authenticateBlock(const byte blockAddress, MFRC522::MIFARE_Key &key) {
    const MFRC522::StatusCode status = rfid.PCD_Authenticate(
        MFRC522::PICC_CMD_MF_AUTH_KEY_A,
        blockAddress,
        &key,
        &rfid.uid
    );
    return status == MFRC522::STATUS_OK;
}

bool RfidRc522Board::readUID(byte *uidBuffer, byte &uidSize) {
    if (uidBuffer == nullptr) {
        return false;
    }

    if (!ensureCardSelected()) {
        return false;
    }

    uidSize = rfid.uid.size;
    memcpy(uidBuffer, rfid.uid.uidByte, uidSize);
    haltCard();
    return true;
}

bool RfidRc522Board::writeUID(const byte *newUid, const byte uidSize) {
    if (newUid == nullptr) {
        return false;
    }
    if (uidSize != 4 && uidSize != 7 && uidSize != 10) {
        return false;
    }
    if (!ensureCardSelected()) {
        return false;
    }
    if (!isMifareClassicCard()) {
        haltCard();
        return false;
    }

    const bool writeOk = rfid.MIFARE_SetUid(const_cast<byte *>(newUid), uidSize, true);
    haltCard();
    return writeOk;
}

bool RfidRc522Board::readUserBlock(const byte blockAddress, byte *blockBuffer16) {
    if (blockBuffer16 == nullptr || !isUserDataBlock(blockAddress)) {
        return false;
    }
    if (!ensureCardSelected()) {
        return false;
    }
    if (!isMifareClassicCard()) {
        haltCard();
        return false;
    }

    MFRC522::MIFARE_Key key{};
    setDefaultMifareKey(key);
    if (!authenticateBlock(blockAddress, key)) {
        haltCard();
        return false;
    }

    byte readBuffer[MIFARE_CLASSIC_BLOCK_SIZE + 2] = {0};
    byte readBufferSize = sizeof(readBuffer);
    const MFRC522::StatusCode status = rfid.MIFARE_Read(blockAddress, readBuffer, &readBufferSize);
    if (status != MFRC522::STATUS_OK) {
        haltCard();
        return false;
    }

    memcpy(blockBuffer16, readBuffer, MIFARE_CLASSIC_BLOCK_SIZE);
    haltCard();
    return true;
}

bool RfidRc522Board::writeUserBlock(const byte blockAddress, const byte *blockBuffer16) {
    if (blockBuffer16 == nullptr || !isUserDataBlock(blockAddress)) {
        return false;
    }
    if (!ensureCardSelected()) {
        return false;
    }
    if (!isMifareClassicCard()) {
        haltCard();
        return false;
    }

    MFRC522::MIFARE_Key key{};
    setDefaultMifareKey(key);
    if (!authenticateBlock(blockAddress, key)) {
        haltCard();
        return false;
    }

    byte writeBuffer[MIFARE_CLASSIC_BLOCK_SIZE] = {0};
    memcpy(writeBuffer, blockBuffer16, MIFARE_CLASSIC_BLOCK_SIZE);
    const MFRC522::StatusCode status = rfid.MIFARE_Write(
        blockAddress,
        writeBuffer,
        sizeof(writeBuffer)
    );
    haltCard();
    return status == MFRC522::STATUS_OK;
}

bool RfidRc522Board::readUserMemory(const byte startBlock, byte *buffer, const byte blockCount) {
    if (buffer == nullptr) {
        return false;
    }
    if (blockCount == 0) {
        return true;
    }
    if (!ensureCardSelected()) {
        return false;
    }
    if (!isMifareClassicCard()) {
        haltCard();
        return false;
    }

    MFRC522::MIFARE_Key key{};
    setDefaultMifareKey(key);

    for (byte i = 0; i < blockCount; i++) {
        const uint16_t blockAddressAsInt = static_cast<uint16_t>(startBlock) + i;
        if (blockAddressAsInt > 255) {
            haltCard();
            return false;
        }

        const byte blockAddress = static_cast<byte>(blockAddressAsInt);
        if (!isUserDataBlock(blockAddress)) {
            haltCard();
            return false;
        }

        if (!authenticateBlock(blockAddress, key)) {
            haltCard();
            return false;
        }

        byte readBuffer[MIFARE_CLASSIC_BLOCK_SIZE + 2] = {0};
        byte readBufferSize = sizeof(readBuffer);
        const MFRC522::StatusCode status = rfid.MIFARE_Read(blockAddress, readBuffer, &readBufferSize);
        if (status != MFRC522::STATUS_OK) {
            haltCard();
            return false;
        }

        memcpy(
            buffer + static_cast<uint16_t>(i) * MIFARE_CLASSIC_BLOCK_SIZE,
            readBuffer,
            MIFARE_CLASSIC_BLOCK_SIZE
        );
    }

    haltCard();
    return true;
}

bool RfidRc522Board::writeUserMemory(const byte startBlock, const byte *buffer, const byte blockCount) {
    if (buffer == nullptr) {
        return false;
    }
    if (blockCount == 0) {
        return true;
    }
    if (!ensureCardSelected()) {
        return false;
    }
    if (!isMifareClassicCard()) {
        haltCard();
        return false;
    }

    MFRC522::MIFARE_Key key{};
    setDefaultMifareKey(key);

    for (byte i = 0; i < blockCount; i++) {
        const uint16_t blockAddressAsInt = static_cast<uint16_t>(startBlock) + i;
        if (blockAddressAsInt > 255) {
            haltCard();
            return false;
        }

        const byte blockAddress = static_cast<byte>(blockAddressAsInt);
        if (!isUserDataBlock(blockAddress)) {
            haltCard();
            return false;
        }

        if (!authenticateBlock(blockAddress, key)) {
            haltCard();
            return false;
        }

        byte writeBuffer[MIFARE_CLASSIC_BLOCK_SIZE] = {0};
        memcpy(
            writeBuffer,
            buffer + static_cast<uint16_t>(i) * MIFARE_CLASSIC_BLOCK_SIZE,
            MIFARE_CLASSIC_BLOCK_SIZE
        );

        const MFRC522::StatusCode status = rfid.MIFARE_Write(
            blockAddress,
            writeBuffer,
            sizeof(writeBuffer)
        );
        if (status != MFRC522::STATUS_OK) {
            haltCard();
            return false;
        }
    }

    haltCard();
    return true;
}

void RfidRc522Board::haltCard() {
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
}
