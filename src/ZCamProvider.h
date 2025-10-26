#pragma once
#include <Arduino.h>
#include "Cameras/ZCamCameraBaseInterface.h"


typedef ZCamCameraBaseInterface* (*ZCamCameraCreator)(const String& params);

class ZCamProvider {
public:
  static bool registerCameraType(const char* key, ZCamCameraCreator fn);
  static ZCamCameraBaseInterface* createCamera(const String& key, const String& params = "");
  static void registerDefaults();

private:
  struct Entry { const char* key; ZCamCameraCreator fn; };
  static const uint8_t kMax = 16;
  static Entry _reg[kMax];
  static uint8_t _count;

  static int8_t findKey(const String& key);
};
