#ifndef PROXIMITY_SENSOR_H
#define PROXIMITY_SENSOR_H
#include <Arduino.h>

class ProximitySensor {
    byte _pin;
    byte _readMode = INPUT;
    byte _activeLogic = HIGH;

    void setPinMode();

public:
    ProximitySensor(const byte pin, const byte readMode): _pin{pin}, _readMode{readMode} {
        setPinMode();
    }

    bool getNewState() const;

    byte getPin() const;
};

#endif //PROXIMITY_SENSOR_H
