#pragma once
#include "../ZCamTypes.h"

class SoftwareSerial;

struct ZCamCameraBaseInterface {
  virtual ~ZCamCameraBaseInterface() {}

  virtual bool init() = 0;

  virtual bool init(const String& params) = 0;

  virtual void setSerial(SoftwareSerial& ser) = 0;

  virtual bool centerPosition() = 0;
  virtual ZCamDataPosition getPosition() const = 0;
};
