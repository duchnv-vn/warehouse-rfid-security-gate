#include <Arduino.h>
#include <SoftwareSerial.h>
#include "ProximitySensor.h"
#include "RfidRc522Board.h"
#include "RfidTag.h"
#include "InterBoardSerialBus.h"

SoftwareSerial link{4, 5};
InterBoardSerialBus bus{link};

constexpr unsigned long SERIAL_BAUD = 9600;
constexpr unsigned int NO_TAG_CHECK_DIRECTION_EXPIRATION = 5000;
constexpr unsigned int HAS_TAG_CHECK_DIRECTION_EXPIRATION = 10000;

constexpr byte SENSOR_COUNT = 2;
byte PROXIMITY_SENSOR_DI_PINS[SENSOR_COUNT] = {2, 3};
ProximitySensor *sensorInstances[SENSOR_COUNT] = {
    new ProximitySensor{PROXIMITY_SENSOR_DI_PINS[0], INPUT_PULLUP},
    new ProximitySensor{PROXIMITY_SENSOR_DI_PINS[1], INPUT_PULLUP},
};

bool pinTriggers[SENSOR_COUNT] = {false, false};

const char *directions[] = {"OUT_IN", "IN_OUT", "UNKNOWN"};
const char *direction = directions[2];

RfidRc522Board *rfidBoard;
byte tagCount = 0;

bool shouldReadUID = true;
bool shouldReadUM = false;
bool shouldRead = false;
unsigned long startScanTimestamp = 0;

byte getDirectionCode() {
    if (direction == directions[0]) {
        return InterBoardSerialBus::DIR_OUT_IN;
    }
    if (direction == directions[1]) {
        return InterBoardSerialBus::DIR_IN_OUT;
    }
    return InterBoardSerialBus::DIR_UNKNOWN;
}

byte getScanningStatusCode(const bool isScanning) {
    if (isScanning) {
        return InterBoardSerialBus::START_SCAN;
    }
    return InterBoardSerialBus::STOP_SCAN;
}

void resetState() {
    shouldRead = false;
    pinTriggers[0] = false;
    pinTriggers[1] = false;
    direction = directions[2];
    tagCount = 0;
}


void startReadTag() {
    if (shouldRead) return;
    shouldRead = true;
    startScanTimestamp = millis();
    bus.sendNewScanningStatus(getScanningStatusCode(shouldRead));
}

void stopReadTag(const bool shouldSendNewScanningStatus = false) {
    shouldRead = false;
    if (shouldSendNewScanningStatus) {
        bus.sendNewScanningStatus(getScanningStatusCode(shouldRead));
    }
}

void checkDirection(const byte pinIdx, const bool state) {
    if (state) {
        pinTriggers[pinIdx] = true;
    }

    const bool allTriggered = pinTriggers[0] && pinTriggers[1];
    if (allTriggered) {
        stopReadTag();
        direction = directions[pinIdx];
        bus.sendNewDirection(getDirectionCode());
        resetState();
        delay(3000);
        return;
    }

    const bool oneTriggered = pinTriggers[0] || pinTriggers[1];
    if (oneTriggered) {
        startReadTag();
    }
}

template<byte pinIdx>
void sensorInterruptImpl() {
    const bool state = sensorInstances[pinIdx]->getNewState();
    checkDirection(pinIdx, state);
}

void (*sensorInterrupt(const byte pinIdx))() {
    switch (pinIdx) {
        case 0: return &sensorInterruptImpl<0>;
        case 1: return &sensorInterruptImpl<1>;
        default: return nullptr;
    }
}

bool attachSensorInterrupt(const byte pin) {
    const byte interruptNumber = digitalPinToInterrupt(pin);
    if (const auto isr = sensorInterrupt(interruptNumber)) {
        attachInterrupt(interruptNumber, isr, CHANGE);
        return true;
    }
    return false;
}

void readTag() {
    if (shouldRead) {
        byte uid[10] = {0};
        byte uidSize = 0;

        const bool tagPresent = rfidBoard->readUID(uid, uidSize);
        if (!tagPresent) {
            return;
        }

        startScanTimestamp = millis();

        RfidTag newTag{};

        if (shouldReadUID) {
            newTag.setUid(uid, uidSize);
        }

        if (shouldReadUM) {
            for (byte block = 1; block < 64; block++) {
                if (((block + 1) % 4) == 0) {
                    continue; // Skip sector trailer block.
                }

                byte data[16] = {0};
                if (!rfidBoard->readUserBlock(block, data)) {
                    continue;
                }

                newTag.addBlockData(block, data);
            }
        }

        ++tagCount;
        bus.sendNewTag(newTag);
    }
}

void checkStopScan() {
    if (shouldRead && (millis() - startScanTimestamp >= (tagCount > 0
                                                             ? HAS_TAG_CHECK_DIRECTION_EXPIRATION
                                                             : NO_TAG_CHECK_DIRECTION_EXPIRATION))) {
        resetState();
        bus.sendNewScanningStatus(getScanningStatusCode(shouldRead));
    }
}

void setup() {
    Serial.begin(SERIAL_BAUD);

    link.begin(9600); // board-to-board serial

    attachSensorInterrupt(PROXIMITY_SENSOR_DI_PINS[0]);
    attachSensorInterrupt(PROXIMITY_SENSOR_DI_PINS[1]);

    rfidBoard = new RfidRc522Board{};

    resetState();
    bus.sendNewScanningStatus(getScanningStatusCode(shouldRead));
}

void loop() {
    readTag();
    checkStopScan();
}
