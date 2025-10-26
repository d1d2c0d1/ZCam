#pragma once
#include "ZCamCameraBaseInterface.h"

class SoftwareSerial;

class ZCamCameraBase : public ZCamCameraBaseInterface
{
public:
  virtual ~ZCamCameraBase() {}

  virtual bool init() override { return true; }

  virtual bool init(const String & /*params*/) override { return init(); }

  void setSerial(SoftwareSerial& ser) override { _serial = &ser; }

  virtual bool centerPosition() override { return true; }
  virtual ZCamDataPosition getPosition() const override { return _data.position; }

protected:
  ZCamData _data;

  SoftwareSerial* _serial = nullptr;
};
