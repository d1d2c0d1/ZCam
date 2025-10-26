#pragma once
#include <Arduino.h>
#include "ZCamTypes.h"
#include "ZCamCondition.h"
#include "ZCamEventInterface.h"
#include "Events/ZCamEventChannel.h"
#include "Events/ZCamEventCombinedChannel.h"

#include "Cameras/ZCamCameraBaseInterface.h"
#include "Cameras/ZCamCameraBase.h"
#include "Cameras/TapotekCamera.h"
#include "Cameras/TapotekKHP290G609Camera.h"

class ZCam
{
public:
  static const uint8_t kMaxEvents = 64;

  ZCam();

  bool addEventHandler(const String &eventName,
                       const String &condition,
                       ZCamCallback cb);

  bool addCombinedEventHandler(const uint8_t *channels,
                               uint8_t count,
                               ZCamCallback cb,
                               uint16_t changeThreshold = 0,
                               const String &name = "channels");

  void tick(uint16_t channels[16]);

  bool registerCamera(ZCamCameraBaseInterface *cam, bool takeOwnership = true);

  // Proxy methods for camera
  bool initCamera();
  bool centerCamera();
  bool setCameraSerial(SoftwareSerial &ser);
  ZCamDataPosition getCameraPos() const;

  ZCamCameraBaseInterface *camera() const { return _camera; }

  ~ZCam();

private:
  ZCamEventInterface *_events[kMaxEvents];
  uint8_t _count;

  ZCamCameraBaseInterface *_camera = nullptr;
  bool _ownCamera = false;

  void propagateCameraToEvents();
};
