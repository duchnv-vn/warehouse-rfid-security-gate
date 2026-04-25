#ifndef PROXIMITY_SENSOR_H
#define PROXIMITY_SENSOR_H
#include <Arduino.h>

class ProximitySensor {
    byte _pin;
    byte _readMode = 0x01;
    byte _activeLogic = INPUT;

public:
    ProximitySensor(const byte pin, const byte readMode): _pin{pin}, _readMode{readMode} {
        setPinMode();
    }

    void setPinMode();

    bool getNewState() const;

    byte getPin() const;
};

#endif //PROXIMITY_SENSOR_H
