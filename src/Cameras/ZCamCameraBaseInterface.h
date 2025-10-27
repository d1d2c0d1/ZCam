#pragma once
#include "../ZCamTypes.h"

class SoftwareSerial;

struct ZCamCameraBaseInterface {
  virtual ~ZCamCameraBaseInterface() {}

  virtual bool init() = 0;

  virtual bool init(const String& params) = 0;

  virtual void setSerial(SoftwareSerial& ser) = 0;

  virtual bool sendPitchToUp();
  virtual bool sendPitchToDown();
  virtual bool sendCenterPitch();
  virtual bool centerPosition() = 0;

  virtual bool sendZoomStop();
  virtual bool sendZoomOut();
  virtual bool sendZoomIn();


  virtual ZCamDataPosition getPosition() const = 0;
};
