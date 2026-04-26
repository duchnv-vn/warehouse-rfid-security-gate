#include <Arduino.h>
#include "ControlButton.h"

void ControlButton::setPinMode() {
    _activeLogic = _readMode == INPUT ? HIGH : LOW;
    pinMode(_pin, _readMode);
}

bool ControlButton::getNewState() const {
    return digitalRead(_pin) == _activeLogic;
}

byte ControlButton::getPin() const {
    return _pin;
}

void ControlButton::subscribeChangeEvent() const {
    const byte interruptNumber = digitalPinToInterrupt(_pin);
    attachInterrupt(interruptNumber, _cbFunc, CHANGE);
}
