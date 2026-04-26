#include "LightTower.h"

void LightTower::setState(const bool state) {
    digitalWrite(_pin, state);
    _state = state;
}
