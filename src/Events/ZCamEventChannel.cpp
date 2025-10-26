#include "ZCamEventChannel.h"

int8_t ZCamEventChannel::parseIndexFromName(const ZCamString &n)
{
  if (!n.startsWith("channel"))
    return 0;
  if (n.length() == 7)
    return 0;
  ZCamString tail = n.substring(7);
  tail.trim();
  int idx = tail.toInt();
  if (idx < 0)
    idx = 0;
  if (idx > 15)
    idx = 15;
  return (int8_t)idx;
}

void ZCamEventChannel::init()
{
  _chIndex = parseIndexFromName(_name);
  _cond.parse(condition());
  _hasLastFired = false;
  _lastFiredVal = 0;
}

void ZCamEventChannel::before(uint16_t /*channels*/[16])
{
}

bool ZCamEventChannel::fire(uint16_t channels[16])
{
  const int16_t v = (int16_t)channels[_chIndex];

  const bool met = _cond.test(v);
  if (!met)
  {
    _hasLastFired = false;
    return false;
  }

  if (_hasLastFired && v == _lastFiredVal)
  {
    return false;
  }

  bool ok = true;
  if (_cb)
  {
    ok = _cb(_name, channels, condition());
  }

  _lastFiredVal = v;
  _hasLastFired = true;

  return ok;
}

void ZCamEventChannel::after(uint16_t /*channels*/[16], bool /*fired*/)
{
}
