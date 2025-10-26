#include "ZCam.h"

class SoftwareSerial;

ZCam::ZCam() : _count(0)
{
  for (uint8_t i = 0; i < kMaxEvents; ++i)
    _events[i] = nullptr;
}

bool ZCam::addEventHandler(const String &eventName,
                           const String &condition,
                           ZCamCallback cb)
{
  if (_count >= kMaxEvents)
    return false;

  ZCamEventInterface *ev = nullptr;
  if (eventName.startsWith("channel"))
  {
    ev = new ZCamEventChannel(eventName, condition, cb);
  }
  else
  {
    ev = new ZCamEventChannel(eventName, condition, cb);
  }
  if (!ev)
    return false;

  ev->attachCamera(_camera);

  ev->init();
  _events[_count++] = ev;
  return true;
}

bool ZCam::addCombinedEventHandler(const uint8_t *channels,
                                   uint8_t count,
                                   ZCamCallback cb,
                                   uint16_t changeThreshold,
                                   const String &name)
{
  if (_count >= kMaxEvents || !channels || count == 0)
    return false;

  String cond;
  cond.reserve(32);
  cond += '[';
  for (uint8_t i = 0; i < count; i++)
  {
    if (i)
      cond += ',';
    cond += String(channels[i]);
  }
  cond += ']';

  auto *ev = new ZCamEventCombinedChannel(name, cond, cb, channels, count, changeThreshold);
  if (!ev)
    return false;

  ev->attachCamera(_camera);
  ev->init();
  _events[_count++] = ev;
  return true;
}

void ZCam::tick(uint16_t channels[16])
{
  for (uint8_t i = 0; i < _count; ++i)
  {
    auto *ev = _events[i];
    if (!ev)
      continue;
    ev->before(channels);
    bool fired = ev->fire(channels);
    ev->after(channels, fired);
  }
}

bool ZCam::setCameraSerial(SoftwareSerial &ser)
{
  if (!_camera)
    return false;
  _camera->setSerial(ser);
  return true;
}

bool ZCam::registerCamera(ZCamCameraBaseInterface *cam, bool takeOwnership)
{
  if (!cam)
    return false;
  if (_camera && _ownCamera)
  {
    delete _camera;
  }
  _camera = cam;
  _ownCamera = takeOwnership;
  propagateCameraToEvents();
  return true;
}

void ZCam::propagateCameraToEvents()
{
  for (uint8_t i = 0; i < _count; ++i)
  {
    if (_events[i])
      _events[i]->attachCamera(_camera);
  }
}

bool ZCam::initCamera()
{
  return _camera ? _camera->init() : false;
}

bool ZCam::centerCamera()
{
  return _camera ? _camera->centerPosition() : false;
}

ZCamDataPosition ZCam::getCameraPos() const
{
  return _camera ? _camera->getPosition() : ZCamDataPosition{};
}

ZCam::~ZCam()
{
  for (uint8_t i = 0; i < _count; ++i)
  {
    delete _events[i];
    _events[i] = nullptr;
  }
  _count = 0;

  if (_camera && _ownCamera)
  {
    delete _camera;
    _camera = nullptr;
  }
}
