#pragma once
#include "../ZCamEvent.h"
#include "../ZCamCondition.h"

class ZCamEventChannel : public ZCamEvent
{
public:
  ZCamEventChannel(const ZCamString &name, const ZCamString &cond, ZCamCallback cb)
      : ZCamEvent(name, cond, cb) {}

  void init() override;
  void before(uint16_t channels[16]) override;
  bool fire(uint16_t channels[16]) override;
  void after(uint16_t channels[16], bool fired) override;

private:
  int8_t _chIndex = 0;
  ZCamCondition _cond;

  bool _hasLastFired = false;
  int16_t _lastFiredVal = 0;

  int8_t parseIndexFromName(const ZCamString &n);
};
