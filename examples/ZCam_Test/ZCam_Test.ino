#include <Arduino.h>
#include <SoftwareSerial.h>
#include <ZCamProvider.h>
#include "SBUS.h"
#include <Cameras/TapotekCamera.h>
#include <ZCam.h>

#define CAMERA_UART_TX 3
#define CAMERA_UART_RX 2

#define LOG_TX 6
#define LOG_RX 5

#define TOTAL_CHANNELS 16

#define ZOOM_CHANNEL 0             // Servo output = 1
#define PITCH_Y_CHANNEL 1          // Servo output = 2
#define PITCH_X_CHANNEL 2          // Servo output = 3
#define PITCH_TOGGLE_CHANNEL 3     // Servo output = 4
#define VIDEO_FORMAT_CHANNEL 4     // Servo output = 5
#define POSITION_CENTER_CHANNEL 5  // Servo output = 6

SoftwareSerial camSerial(CAMERA_UART_TX, CAMERA_UART_RX);
SBUS sbus(Serial);

ZCam zcam;
uint16_t channels[TOTAL_CHANNELS];
uint16_t lastChannels[TOTAL_CHANNELS];

const uint8_t pitchChannels[2] = { PITCH_X_CHANNEL, PITCH_Y_CHANNEL };

bool failsafe;
bool lostFrame;
bool brokenSignal = false;

enum Move { MOVE_STOP,
            MOVE_UP,
            MOVE_DOWN,
            MOVE_LEFT,
            MOVE_RIGHT };

bool onVideoFormatChange(const String& eventName, uint16_t ch[TOTAL_CHANNELS], const String& condition) {
  uint16_t channelPWM = ch[VIDEO_FORMAT_CHANNEL];
  TapotekCamera* cam = (TapotekCamera*)zcam.camera();

  if (cam) {
    if (channelPWM >= 1670) {
      cam->sendChangeIRColorMode();
    } else if (channelPWM <= 300) {
      cam->sendChangeWindowMode();
    }
  } else {
    return false;
  }

  return true;
}

bool onZoomChange(const String& eventName, uint16_t ch[TOTAL_CHANNELS], const String& condition) {
  uint16_t channelPWM = ch[ZOOM_CHANNEL];
  TapotekCamera* cam = (TapotekCamera*)zcam.camera();

  if (cam) {
    if (channelPWM >= 1670) {
      cam->sendZoomIn();
    } else if (channelPWM >= 600 && channelPWM <= 1300) {
      cam->sendZoomStop();
    } else if (channelPWM <= 300) {
      cam->sendZoomOut();
    }
  } else {
    return false;
  }

  return true;
}

bool onPitchToggleChange(const String& eventName, uint16_t ch[TOTAL_CHANNELS], const String& condition) {
  uint16_t channelPWM = ch[PITCH_TOGGLE_CHANNEL];
  TapotekCamera* cam = (TapotekCamera*)zcam.camera();

  if (cam) {
    if (channelPWM >= 1670) {
      cam->sendPitchToUp();
    } else if (channelPWM >= 600 && channelPWM <= 1300) {
      cam->sendCenterPitch();
    } else if (channelPWM <= 300) {
      cam->sendPitchToDown();
    }
  } else {
    return false;
  }

  return true;
}

bool onPositionCenterFired(const String& eventName, uint16_t ch[TOTAL_CHANNELS], const String& condition) {

  TapotekCamera* cam = (TapotekCamera*)zcam.camera();

  if (cam) {
    cam->sendCenterPitch();
  }

  return true;
}

bool onPitchXYChange(const String& /*eventName*/, uint16_t ch[], const String& /*condition*/) {
  const int center = 930;
  const int dead = 200;

  int y = (int)ch[PITCH_Y_CHANNEL];
  int x = (int)ch[PITCH_X_CHANNEL];

  int dy = y - center;
  int dx = x - center;

  bool yActive = abs(dy) > dead;
  bool xActive = abs(dx) > dead;

  Move m = MOVE_STOP;
  if (yActive || xActive) {
    if (yActive && (!xActive || abs(dy) >= abs(dx))) {
      m = (dy > 0) ? MOVE_UP : MOVE_DOWN;
    } else {
      m = (dx > 0) ? MOVE_LEFT : MOVE_RIGHT;
    }
  }

  static Move last = MOVE_STOP;
  static unsigned long lastSent = 0;
  unsigned long now = millis();

  const unsigned long keepAliveMs = 300;

  if (m != last || (now - lastSent) > keepAliveMs) {
    if (auto* cam = static_cast<TapotekCamera*>(zcam.camera())) {
      switch (m) {
        case MOVE_UP: cam->sendGimbalMove("up"); break;
        case MOVE_DOWN: cam->sendGimbalMove("down"); break;
        case MOVE_LEFT: cam->sendGimbalMove("right"); break;
        case MOVE_RIGHT: cam->sendGimbalMove("left"); break;
        case MOVE_STOP: cam->sendGimbalMove("stop"); break;
      }
    }
    last = m;
    lastSent = now;
  }

  return true;
}

void setup() {

  camSerial.begin(115200);
  sbus.begin();

  delay(100);

  for (uint8_t i = 0; i < 16; ++i) channels[i] = 0;

  ZCamProvider::registerDefaults();

  TapotekCamera* cam = ZCamProvider::createCamera(F("tapotek.KHP290G609"), F("baud=115200;pin=2,3"));

  if (cam) {
    zcam.registerCamera(cam, /*takeOwnership=*/true);
    zcam.setCameraSerial(camSerial);
    zcam.initCamera();
  }

  // Registering event handlers for channels changed values
  zcam.addEventHandler("channel" + String(VIDEO_FORMAT_CHANNEL), "", onVideoFormatChange);
  zcam.addEventHandler("channel" + String(ZOOM_CHANNEL), "", onZoomChange);
  zcam.addEventHandler("channel" + String(PITCH_TOGGLE_CHANNEL), "", onPitchToggleChange);
  zcam.addEventHandler("channel" + String(POSITION_CENTER_CHANNEL), ">=1650", onPositionCenterFired);
  zcam.addCombinedEventHandler(pitchChannels, 2, +onPitchXYChange, /*changeThreshold=*/5, "pitchXY");
}

inline void print4u(Stream& s, uint16_t v) {
  if (v < 10) s.print(F("000"));
  else if (v < 100) s.print(F("00"));
  else if (v < 1000) s.print(F("0"));
  s.print(v);
}

void loop() {
  if (sbus.read(channels, &failsafe, &lostFrame)) {
    brokenSignal = false;  // If channel FS or detected is broken signal
    uint8_t countChangedChannels = 0;

    for (int i = 0; i < 16; i++) {
      if (lastChannels[i] != channels[i]) {
        ++countChangedChannels;
      }

      lastChannels[i] = channels[i];
    }

    if (countChangedChannels > 9) {
      brokenSignal = true;
    }

    if (!failsafe && !brokenSignal) {
      zcam.tick(channels);
    }
  }
}