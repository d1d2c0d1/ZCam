#pragma once
#include "ZCamCameraBase.h"

class TapotekCamera : public ZCamCameraBase
{
public:
  using ZCamCameraBase::init;
  virtual bool init() override;

  bool sendCommandWithCRC(const char *cmdWithoutCRC);
  bool sendCommandWithCRC(const char *cmdWithoutCRC, String &outResponse, uint16_t timeoutMs = 100);

  bool sendPitchAngle(int16_t angle, uint8_t speed);
  bool sendPitchToUp();
  bool sendPitchToDown();
  bool sendCenterPitch();
  virtual bool centerPosition() override;
  bool sendGimbalMove(const String& direction = "stop");

  bool sendZoomStop();
  bool sendZoomOut();
  bool sendZoomIn();

  bool sendChangeIRColorMode();
  bool sendChangeWindowMode();

protected:
  uint16_t calculateChecksum(const char *cmd);
  bool readResponseLine(String &out, uint16_t timeoutMs);
  bool matchesEchoWithSwappedAddrs(const String &sent, const String &resp);
  bool isErrorResponse(const String &resp, uint8_t *errCodeHex /*nullable*/);
  bool extractAddrs(const String &frame, String &src2, String &dst2);
  static void trimLine(String &s);
};
