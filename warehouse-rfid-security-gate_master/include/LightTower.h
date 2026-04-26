#ifndef LIGHT_TOWER_H
#define LIGHT_TOWER_H

#include <Arduino.h>

class LightTower {
    const byte _pin;
    bool _state = false;

public:
    LightTower(const byte pin): _pin{pin} {
        pinMode(_pin, OUTPUT);
        _state = false;
        setState(false);
    }

    void setState(bool state);
};


#endif //LIGHT_TOWER_H
