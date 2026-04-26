#ifndef CONTROL_BUTTON_H
#define CONTROL_BUTTON_H

#include <Arduino.h>

class ControlButton {
    const byte _pin;
    byte _readMode = INPUT;
    byte _activeLogic = HIGH;

    void (*_cbFunc)();

    void setPinMode();

public:
    ControlButton(const byte pin, const byte readMode, void (*cbFunc)()):
        _pin{pin},
        _readMode{readMode},
        _cbFunc{cbFunc} {
        setPinMode();
    }

    bool getNewState() const;

    byte getPin() const;

    void subscribeChangeEvent() const;
};


#endif //CONTROL_BUTTON_H
