#include "ProximitySensor.h"

void ProximitySensor::setPinMode() {
    _activeLogic = _readMode == INPUT ? HIGH : LOW;
    pinMode(_pin, _readMode);
}

bool ProximitySensor::getNewState() const {
    return digitalRead(_pin) == _activeLogic;
}

byte ProximitySensor::getPin() const {
    return _pin;
}
