#ifndef RFID_RC522_RST_PIN
#define RFID_RC522_RST_PIN 9 // Reset pin
#endif

#ifndef RFID_RC522_SDA_SS_PIN
#define RFID_RC522_SDA_SS_PIN 10 // SPI slave select / chip select pin
#endif

#ifndef RFID_RC522_MOSI_PIN
#define RFID_RC522_MOSI_PIN 11 // Master Out, Slave In. This is the SPI data line that sends data from Arduino to RC522.
#endif

#ifndef RFID_RC522_MISO_PIN
#define RFID_RC522_MISO_PIN 12 // Master In, Slave Out. This is the SPI data line that sends data from RC522 to Arduino.
#endif

#ifndef RFID_RC522_SCK_PIN
#define RFID_RC522_SCK_PIN 13 // SPI clock. The Arduino provides the timing clock for SPI communication.
#endif

#ifndef RFID_RC522BOARD_H
#define RFID_RC522BOARD_H

#include <SPI.h>
#include <MFRC522.h>

class RfidRc522Board {
    MFRC522 rfid{RFID_RC522_SDA_SS_PIN,RFID_RC522_RST_PIN};

    static bool isSectorTrailerBlock(byte blockAddress);

    static bool isUserDataBlock(byte blockAddress);

    static void setDefaultMifareKey(MFRC522::MIFARE_Key &key);

    bool ensureCardSelected();

    bool isMifareClassicCard() const;

    bool authenticateBlock(byte blockAddress, MFRC522::MIFARE_Key &key);

public:
    static constexpr byte MIFARE_CLASSIC_BLOCK_SIZE = 16;

    RfidRc522Board() {
        SPI.begin();
        rfid.PCD_Init();
    }

    bool isNewCardPresent();

    bool readCardSerial();

    void reset();

    MFRC522::Uid getUID() const;

    bool readUID(byte *uidBuffer, byte &uidSize);

    bool writeUID(const byte *newUid, byte uidSize);

    bool readUserBlock(byte blockAddress, byte *blockBuffer16);

    bool writeUserBlock(byte blockAddress, const byte *blockBuffer16);

    bool readUserMemory(byte startBlock, byte *buffer, byte blockCount);

    bool writeUserMemory(byte startBlock, const byte *buffer, byte blockCount);

    void haltCard();
};


#endif //RFID_RC522BOARD_H
