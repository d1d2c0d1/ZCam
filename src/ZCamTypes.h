#pragma once
#include <Arduino.h>

using ZCamString = String;

struct ZCamDataPosition {
  int16_t x = 0;
  int16_t y = 0;
  int16_t z = 0;
};

struct ZCamData {
  ZCamDataPosition position;
};