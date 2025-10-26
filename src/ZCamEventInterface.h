#pragma once
#include <Arduino.h>
#include "ZCamTypes.h"
#include "Cameras/ZCamCameraBaseInterface.h"

typedef bool (*ZCamCallback)(const String &eventName,
                             uint16_t channels[16],
                             const String &condition);

struct ZCamEventInterface
{
  virtual ~ZCamEventInterface() {}

  virtual void init() = 0;
  virtual void before(uint16_t channels[16]) = 0;
  virtual bool fire(uint16_t channels[16]) = 0;
  virtual void after(uint16_t channels[16], bool fired) = 0;

  virtual const String &name() const = 0;
  virtual const String &condition() const = 0;

  // ⬇️ Новое: камера, доступная событиям
  virtual void attachCamera(ZCamCameraBaseInterface *cam) = 0;
  virtual ZCamCameraBaseInterface *camera() const = 0;
};
