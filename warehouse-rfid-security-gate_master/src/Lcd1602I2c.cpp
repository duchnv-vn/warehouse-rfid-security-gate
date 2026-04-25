#include "Lcd1602I2c.h"

namespace {
    byte getTextLenWithinColumns(const char *text, const byte columns) {
        if (text == nullptr) {
            return 0;
        }

        byte len = 0;
        while (text[len] != '\0' && len < columns) {
            len++;
        }
        return len;
    }
}

Lcd1602I2c::Lcd1602I2c(const byte i2cAddress, const byte columns, const byte rows): _lcd{i2cAddress, columns, rows},
    _columns{columns},
    _rows{rows} {
}

void Lcd1602I2c::begin() {
    Wire.begin();
    _lcd.init();
    _lcd.clear();
    _lcd.backlight();
    _initialized = true;
}

bool Lcd1602I2c::isInitialized() const {
    return _initialized;
}

void Lcd1602I2c::clear() {
    _lcd.clear();
}

void Lcd1602I2c::home() {
    _lcd.home();
}

void Lcd1602I2c::setBacklight(const bool enabled) {
    if (enabled) {
        _lcd.backlight();
        return;
    }
    _lcd.noBacklight();
}

void Lcd1602I2c::setCursor(byte column, byte row) {
    if (_columns == 0 || _rows == 0) {
        return;
    }
    if (column >= _columns) {
        column = static_cast<byte>(_columns - 1);
    }
    if (row >= _rows) {
        row = static_cast<byte>(_rows - 1);
    }
    _lcd.setCursor(column, row);
}

void Lcd1602I2c::print(const char *text) {
    if (text == nullptr) {
        return;
    }
    _lcd.print(text);
}

void Lcd1602I2c::printAt(const byte column, const byte row, const char *text) {
    setCursor(column, row);
    print(text);
}

void Lcd1602I2c::writeLine(const byte row, const char *text, const bool clearRemaining) {
    writeLineLeft(row, text, clearRemaining);
}

void Lcd1602I2c::writeLineAligned(
    const byte row,
    const char *text,
    const LineAlign align,
    const bool clearLine
) {
    if (_rows == 0 || _columns == 0) {
        return;
    }

    const byte textLen = getTextLenWithinColumns(text, _columns);
    byte startColumn = 0;

    switch (align) {
        case LineAlign::CENTER:
            startColumn = static_cast<byte>((_columns - textLen) / 2);
            break;
        case LineAlign::RIGHT:
            startColumn = static_cast<byte>(_columns - textLen);
            break;
        case LineAlign::LEFT:
        default:
            startColumn = 0;
            break;
    }

    if (clearLine) {
        setCursor(0, row);
        for (byte i = 0; i < _columns; i++) {
            _lcd.write(' ');
        }
    }

    if (textLen == 0) {
        return;
    }

    setCursor(startColumn, row);
    for (byte i = 0; i < textLen; i++) {
        _lcd.write(text[i]);
    }
}

void Lcd1602I2c::writeLineLeft(const byte row, const char *text, const bool clearLine) {
    writeLineAligned(row, text, LineAlign::LEFT, clearLine);
}

void Lcd1602I2c::writeLineCenter(const byte row, const char *text, const bool clearLine) {
    writeLineAligned(row, text, LineAlign::CENTER, clearLine);
}

void Lcd1602I2c::writeLineRight(const byte row, const char *text, const bool clearLine) {
    writeLineAligned(row, text, LineAlign::RIGHT, clearLine);
}
