#include <Arduino.h>
#include <SoftwareSerial.h>
#include "RfidTag.h"
#include "Lcd1602I2c.h"
#include "InterBoardSerialBus.h"
#include "ControlButton.h"
#include "LightTower.h"

SoftwareSerial link{4, 5};
InterBoardSerialBus bus{link};

byte RESET_BUTTON_DI_PIN = 3;
byte resetDataTriggerCount = 0;

constexpr unsigned long SERIAL_BAUD = 9600;
constexpr byte MAX_TAGS = 20;
const char *directions[] = {"OUT_IN", "IN_OUT", "UNKNOWN"};
const char *direction = directions[2];
InterBoardSerialBus::TagUid tags[MAX_TAGS] = {};
byte tagCount = 0;

byte scanStatus = InterBoardSerialBus::STOP_SCAN;

Lcd1602I2c lcd{0x27, 16, 2};

auto lightTower = new LightTower{6};

void resetData() {
    direction = directions[2];
    tagCount = 0;
    memset(tags, 0, sizeof(tags));
}

void renderResetDataRow() {
    lcd.writeLineAligned(0, "RESET DATA", Lcd1602I2c::LineAlign::CENTER);
    lcd.writeLineAligned(1, "", Lcd1602I2c::LineAlign::LEFT);
}

void renderStopScanRow() {
    lcd.writeLineAligned(0, "STOP SCAN", Lcd1602I2c::LineAlign::CENTER);
    lcd.writeLineAligned(1, "", Lcd1602I2c::LineAlign::LEFT);
}

void renderStartScanRow(const bool clear2ndRow = false) {
    char line[17] = {0};
    snprintf(line, sizeof(line), "SCAN TAGS:%u", tagCount);
    lcd.writeLineAligned(0, line, Lcd1602I2c::LineAlign::CENTER);
    if (clear2ndRow) {
        lcd.writeLineAligned(1, "", Lcd1602I2c::LineAlign::LEFT);
    }
}

void renderFinishScanRow() {
    char line[17] = {0};
    snprintf(line, sizeof(line), "%s T:%u", direction, tagCount);
    lcd.writeLineAligned(0, line, Lcd1602I2c::LineAlign::CENTER);
    lcd.writeLineAligned(1, "", Lcd1602I2c::LineAlign::LEFT);
}

void renderUID(const char *uidHex) {
    lcd.writeLineAligned(1, uidHex, Lcd1602I2c::LineAlign::CENTER);
}

void onNewResetButtonTrigger() {
    resetDataTriggerCount++;
}

auto resetButton = new ControlButton{RESET_BUTTON_DI_PIN, INPUT_PULLUP, onNewResetButtonTrigger};

bool isUIDDuplicate(const InterBoardSerialBus::TagUid &newTag) {
    if (newTag.uidSize > RfidTag::MAX_UID_SIZE) return false;

    for (const auto &tag: tags) {
        if (tag.uidSize != newTag.uidSize) continue;
        if (memcmp(newTag.uid, tag.uid, newTag.uidSize) == 0) return true;
    }
    return false;
}

void onNewTransaction() {
    renderFinishScanRow();

    Serial.print(F("Direction: "));
    Serial.println(direction);
    Serial.print(F("Tag count: "));
    Serial.println(tagCount);
    resetData();
    lightTower->setState(false);
}

const char *directionCodeToText(const byte directionCode) {
    switch (directionCode) {
        case InterBoardSerialBus::DIR_OUT_IN:
            return "OUT_IN";
        case InterBoardSerialBus::DIR_IN_OUT:
            return "IN_OUT";
        default:
            return "UNKNOWN";
    }
}

const char *scanningStatusToText(const byte scanningStatusCode) {
    switch (scanningStatusCode) {
        case InterBoardSerialBus::START_SCAN:
            return "START_SCAN";
        case InterBoardSerialBus::STOP_SCAN:
            return "STOP_SCAN";
        default:
            return "UNKNOWN";
    }
}

void onReceiveNewDirection(const byte, const byte *payload, const byte payloadLength) {
    byte receivedDirection = InterBoardSerialBus::DIR_UNKNOWN;
    if (!InterBoardSerialBus::parseNewDirection(payload, payloadLength, receivedDirection)) {
        Serial.println(F("RX DIR parse error"));
        return;
    }

    direction = directionCodeToText(receivedDirection);
    onNewTransaction();
}

void onReceiveNewTag(const byte, const byte *payload, const byte payloadLength) {
    InterBoardSerialBus::TagUid receivedTag{};
    if (!InterBoardSerialBus::parseNewTag(payload, payloadLength, receivedTag)) {
        Serial.println(F("RX TAG parse error"));
        return;
    }

    char uidHex[30] = {0};
    if (!RfidTag::uidToHex(receivedTag.uid, receivedTag.uidSize, uidHex, sizeof(uidHex))) {
        Serial.println(F("RX TAG uid format error"));
        return;
    }

    if (tagCount < MAX_TAGS and !isUIDDuplicate(receivedTag)) {
        tags[tagCount++] = receivedTag;
        renderUID(uidHex);
        renderStartScanRow();
    }
}

void onReceiveNewScanningStatus(const byte, const byte *payload, const byte payloadLength) {
    byte receivedScanningStatus = InterBoardSerialBus::SCAN_UNKNOWN;

    if (!InterBoardSerialBus::parseNewDirection(payload, payloadLength, receivedScanningStatus)) {
        Serial.println(F("RX SCAN parse error"));
        return;
    }

    scanStatus = receivedScanningStatus;

    if (receivedScanningStatus == InterBoardSerialBus::START_SCAN) {
        renderStartScanRow();
        lightTower->setState(true);
    } else {
        resetData();
        renderStopScanRow();
        lightTower->setState(false);
    }
}

void checkResetData() {
    if (resetDataTriggerCount >= 2) {
        resetDataTriggerCount = 0;
        renderResetDataRow();
        resetData();
        delay(1000);

        if (scanStatus == InterBoardSerialBus::START_SCAN) {
            renderStartScanRow(true);
        } else {
            renderStopScanRow();
        }
    }
}

void setup() {
    Serial.begin(SERIAL_BAUD);

    link.begin(9600); // board-to-board serial
    bus.on(InterBoardSerialBus::MSG_NEW_DIRECTION, onReceiveNewDirection);
    bus.on(InterBoardSerialBus::MSG_NEW_TAG, onReceiveNewTag);
    bus.on(InterBoardSerialBus::NSG_NEW_SCAN_STATUS, onReceiveNewScanningStatus);

    resetButton->subscribeChangeEvent();

    lcd.begin();
    lcd.writeLineAligned(0, "RFID GATE", Lcd1602I2c::LineAlign::CENTER);
    lcd.writeLineAligned(1, "WELCOME", Lcd1602I2c::LineAlign::CENTER);
    delay(2000);
    lcd.clear();
    tagCount = 0;
    renderStopScanRow();
}

void loop() {
    bus.poll();
    checkResetData();
}
