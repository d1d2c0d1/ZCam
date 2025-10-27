#include <SoftwareSerial.h>
#include "TapotekCamera.h"

bool TapotekCamera::init()
{
  _data.position.x = 0;
  _data.position.y = 0;
  _data.position.z = 0;
  return true;
}

uint16_t TapotekCamera::calculateChecksum(const char *cmd)
{
  uint16_t sum = 0;
  for (size_t i = 0; cmd[i] != '\0'; i++)
  {
    sum += (uint8_t)cmd[i];
  }
  return sum;
}

bool TapotekCamera::sendCommandWithCRC(const char *cmdWithoutCRC)
{
  String dummy;
  return sendCommandWithCRC(cmdWithoutCRC, dummy, /*timeoutMs=*/100);
}

bool TapotekCamera::sendCommandWithCRC(const char *cmdWithoutCRC, String &outResponse, uint16_t timeoutMs)
{
  // Debounce variable
  unsigned long now = millis();

  if (!_serial)
    return false;

  uint16_t checksum = calculateChecksum(cmdWithoutCRC);
  uint8_t lowByte = checksum & 0xFF;

  char fullCmd[64];
  snprintf(fullCmd, sizeof(fullCmd), "%s%02X", cmdWithoutCRC, lowByte);

  // Debounce continued
  if ((now - lastCmdTime < 1000) && (strcmp(fullCmd, _lastCmd) == 0))
  {
    return false;
  }

  // Sending command to Camera
  _serial->println(fullCmd);

  if (_logSerial)
  {
    _logSerial->print("> TapotekCamera::sendCommandWithCRC( \"");
    _logSerial->print(fullCmd);
    _logSerial->print("\" );");
    _logSerial->println();
  }

  // delay(100);

  // String resp;
  // if (!readResponseLine(resp, timeoutMs))
  // {
  //   return false;
  // }
  // outResponse = resp;

  // uint8_t err = 0;
  // if (isErrorResponse(resp, &err))
  // {
  //   return false;
  // }

  // if (!matchesEchoWithSwappedAddrs(String(fullCmd), resp))
  // {
  //   return false;
  // }

  lastCmdTime = millis();
  strncpy(_lastCmd, fullCmd, sizeof(_lastCmd));

  return true;
}

/** Posititons */

bool TapotekCamera::sendPitchAngle(int16_t angle, uint8_t speed)
{
  char angleStr[5];
  uint16_t val = (uint16_t)angle;

  angleStr[0] = "0123456789ABCDEF"[val >> 12];
  angleStr[1] = "0123456789ABCDEF"[(val >> 8) & 0x0F];
  angleStr[2] = "0123456789ABCDEF"[(val >> 4) & 0x0F];
  angleStr[3] = "0123456789ABCDEF"[val & 0x0F];
  angleStr[4] = '\0';

  char cmd[32];
  snprintf(cmd, sizeof(cmd), "#tpUG6wGIP%s%02X", angleStr, speed);

  return sendCommandWithCRC(cmd);
}

bool TapotekCamera::sendPitchToUp()
{
  return sendPitchAngle(-1500, 0x29);
}

bool TapotekCamera::sendPitchToDown()
{
  return sendPitchAngle(9000, 0x29);
}

bool TapotekCamera::sendCenterPitch()
{
  return sendCommandWithCRC("#TPUG2wPTZ05");
}

// Proxy for support ZCam->centerPosition
bool TapotekCamera::centerPosition()
{
  return sendCenterPitch();
}

bool TapotekCamera::sendGimbalMove(const String &direction)
{
  String code = "00";

  if (direction.equalsIgnoreCase("up"))
    code = "01";
  else if (direction.equalsIgnoreCase("down"))
    code = "02";
  else if (direction.equalsIgnoreCase("left"))
    code = "03";
  else if (direction.equalsIgnoreCase("right"))
    code = "04";
  else
    code = "00";

  String cmd = "#TPUG2wPTZ" + code;

  return sendCommandWithCRC(cmd.c_str());
}

/***************
 * -= ZOOM =- *
 **************/

bool TapotekCamera::sendZoomStop()
{
  return sendCommandWithCRC("#TPUM2wZMC00");
}

bool TapotekCamera::sendZoomOut()
{
  return sendCommandWithCRC("#TPUM2wZMC01");
}

bool TapotekCamera::sendZoomIn()
{
  return sendCommandWithCRC("#TPUM2wZMC02");
}

/**
 * IMAGE FORMAT
 */

bool TapotekCamera::sendChangeIRColorMode()
{
  return sendCommandWithCRC("#TPUD2wIMG0A");
}

bool TapotekCamera::sendChangeWindowMode()
{
  return sendCommandWithCRC("#TPUD2wPIP0A");
}

/**
 * HELPERS
 */

void TapotekCamera::trimLine(String &s)
{
  while (s.length() && (s[s.length() - 1] == '\r' || s[s.length() - 1] == '\n'))
    s.remove(s.length() - 1);
  while (s.length() && (s[0] == '\r' || s[0] == '\n'))
    s.remove(0, 1);
}

bool TapotekCamera::readResponseLine(String &out, uint16_t timeoutMs)
{
  out = "";
  if (!_serial)
    return false;
  unsigned long endAt = millis() + timeoutMs;

  while (millis() < endAt)
  {
    if (_serial->available())
    {
      int c = _serial->read();
      if (c < 0)
        continue;
      if ((char)c == '#')
      {
        out += (char)c;
        break;
      }
    }
  }
  if (out.length() == 0)
    return false;

  while (millis() < endAt)
  {
    while (_serial->available())
    {
      int c = _serial->read();
      if (c < 0)
        continue;
      out += (char)c;
      if ((char)c == '\n')
      {
        trimLine(out);
        return true;
      }
    }
  }

  trimLine(out);
  return out.length() > 0;
}

bool TapotekCamera::extractAddrs(const String &frame, String &src2, String &dst2)
{
  if (frame.length() < 5)
    return false;
  if (frame[0] != '#')
    return false;
  src2 = frame.substring(1, 3);
  dst2 = frame.substring(3, 5);
  return true;
}

bool TapotekCamera::matchesEchoWithSwappedAddrs(const String &sent, const String &resp)
{
  String s_src, s_dst, r_src, r_dst;
  if (!extractAddrs(sent, s_src, s_dst))
    return false;
  if (!extractAddrs(resp, r_src, r_dst))
    return false;
  if (!r_src.equalsIgnoreCase(s_dst))
    return false;
  if (!r_dst.equalsIgnoreCase(s_src))
    return false;
  return true;
}

bool TapotekCamera::isErrorResponse(const String &resp, uint8_t *errCodeHex)
{
  int idx = resp.indexOf(F("ERE!!"));
  if (idx < 0)
    return false;
  uint8_t code = 0;
  if (idx + 5 + 2 <= resp.length())
  {
    char c1 = resp[idx + 5];
    char c2 = resp[idx + 6];
    auto hexVal = [](char c) -> int
    {
      if (c >= '0' && c <= '9')
        return c - '0';
      if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
      if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
      return -1;
    };
    int h1 = hexVal(c1), h2 = hexVal(c2);
    if (h1 >= 0 && h2 >= 0)
      code = (uint8_t)((h1 << 4) | h2);
  }
  if (errCodeHex)
    *errCodeHex = code;
  return true;
}