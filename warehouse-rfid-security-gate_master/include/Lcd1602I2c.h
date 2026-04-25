#ifndef LCD1602_I2C_H
#define LCD1602_I2C_H

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

class Lcd1602I2c {
public:
    enum class LineAlign : byte {
        LEFT,
        CENTER,
        RIGHT
    };

private:
    LiquidCrystal_I2C _lcd;
    byte _columns;
    byte _rows;
    bool _initialized = false;

public:
    Lcd1602I2c(byte i2cAddress = 0x27, byte columns = 16, byte rows = 2);

    void begin();

    bool isInitialized() const;

    void clear();

    void home();

    void setBacklight(bool enabled);

    void setCursor(byte column, byte row);

    void print(const char *text);

    void printAt(byte column, byte row, const char *text);

    void writeLine(byte row, const char *text, bool clearRemaining = true);

    void writeLineAligned(byte row, const char *text, LineAlign align, bool clearLine = true);

    void writeLineLeft(byte row, const char *text, bool clearLine = true);

    void writeLineCenter(byte row, const char *text, bool clearLine = true);

    void writeLineRight(byte row, const char *text, bool clearLine = true);
};

#endif //LCD1602_I2C_H
