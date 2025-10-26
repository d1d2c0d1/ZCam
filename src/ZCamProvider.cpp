#include "ZCamProvider.h"
#include "Cameras/TapotekKHP290G609Camera.h"

ZCamProvider::Entry ZCamProvider::_reg[ZCamProvider::kMax];
uint8_t ZCamProvider::_count = 0;

bool ZCamProvider::registerCameraType(const char* key, ZCamCameraCreator fn) {
  if (!key || !fn) return false;
  for (uint8_t i=0; i<_count; ++i) {
    if (strcmp(_reg[i].key, key) == 0) {
      _reg[i].fn = fn;
      return true;
    }
  }
  if (_count >= kMax) return false;
  _reg[_count++] = Entry{ key, fn };
  return true;
}

int8_t ZCamProvider::findKey(const String& key) {
  for (uint8_t i=0; i<_count; ++i) {
    if (key.equalsIgnoreCase(_reg[i].key)) return (int8_t)i;
  }
  return -1;
}

ZCamCameraBaseInterface* ZCamProvider::createCamera(const String& key, const String& params) {
  int8_t idx = findKey(key);
  if (idx < 0) return nullptr;

  ZCamCameraCreator fn = _reg[idx].fn;
  if (!fn) return nullptr;

  ZCamCameraBaseInterface* cam = fn(params);
  return cam;
}

void ZCamProvider::registerDefaults() {
  registerCameraType("tapotek.KHP290G609",
    [](const String& params)->ZCamCameraBaseInterface* {
      auto* cam = new TapotekKHP290G609Camera();
      if (cam) {
        cam->init(params);
      }
      return cam;
    }
  );
}
