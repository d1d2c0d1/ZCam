#pragma once
#include "../ZCamEvent.h"

class ZCamEventCombinedChannel : public ZCamEvent {
public:
  // channels[] — список индексов каналов (0..15)
  // count      — длина списка (до kMaxWatch)
  // delta      — порог изменения, при котором считаем, что канал «изменился»
  ZCamEventCombinedChannel(const String& name,
                    const String& cond,
                    ZCamCallback cb,
                    const uint8_t* channels,
                    uint8_t count,
                    uint16_t delta = 0)
  : ZCamEvent(name, cond, cb), _count(count), _delta(delta)
  {
    if (_count > kMaxWatch) _count = kMaxWatch;
    for (uint8_t i = 0; i < _count; ++i) _watch[i] = channels[i];
  }

  void init() override {
    _hasPrev = false;
    for (uint8_t i=0;i<kMaxWatch;i++) _prev[i] = 0;
  }

  void before(uint16_t /*channels*/[16]) override { /* no-op */ }

  bool fire(uint16_t channels[16]) override {
    bool changed = false;

    if (!_hasPrev) {
      for (uint8_t i=0;i<_count;i++) _prev[i] = channels[_watch[i]];
      _hasPrev = true;
      return false;
    }

    for (uint8_t i=0;i<_count;i++) {
      uint16_t cur  = channels[_watch[i]];
      uint16_t prev = _prev[i];
      if (_delta == 0) {
        if (cur != prev) { changed = true; break; }
      } else {
        int diff = (int)cur - (int)prev;
        if (diff < 0) diff = -diff;
        if (diff > _delta) { changed = true; break; }
      }
    }

    if (!changed) return false;

    for (uint8_t i=0;i<_count;i++) _prev[i] = channels[_watch[i]];

    if (_cb) return _cb(_name, channels, _cond);
    return true;
  }

  void after(uint16_t /*channels*/[16], bool /*fired*/) override { /* no-op */ }

private:
  static const uint8_t kMaxWatch = 8;
  uint8_t  _watch[kMaxWatch] = {0};
  uint16_t _prev[kMaxWatch]  = {0};
  uint8_t  _count            = 0;
  uint16_t _delta            = 0;
  bool     _hasPrev          = false;
};
