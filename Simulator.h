#pragma once
#include <Arduino.h>

class Simulator {
public:
  void begin();
  void update();

private:
  uint32_t _lastUpdateMs = 0;
  float _phase = 0.0f;
};
