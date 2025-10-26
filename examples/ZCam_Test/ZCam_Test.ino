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

#define VIDEO_FORMAT_CHANNEL 4
#define ZOOM_CHANNEL 0
#define PITCH_TOGGLE_CHANNEL 3
#define PITCH_Y_CHANNEL 1
#define PITCH_X_CHANNEL 2

SoftwareSerial logSerial(LOG_TX, LOG_RX);
SoftwareSerial camSerial(CAMERA_UART_TX, CAMERA_UART_RX);
SBUS sbus(Serial);

ZCam zcam;
uint16_t channels[TOTAL_CHANNELS];

const uint8_t pitchChannels[2] = { PITCH_X_CHANNEL, PITCH_Y_CHANNEL };

bool failsafe;
bool lostFrame;

enum Move { MOVE_STOP,
            MOVE_UP,
            MOVE_DOWN,
            MOVE_LEFT,
            MOVE_RIGHT };

bool onVideoFormatChange(const String& eventName, uint16_t ch[TOTAL_CHANNELS], const String& condition) {
  uint16_t channelPWM = ch[VIDEO_FORMAT_CHANNEL];
  TapotekCamera* cam = (TapotekCamera*)zcam.camera();

  if (cam) {
    if (channelPWM >= 1500) {
      cam->sendChangeIRColorMode();
    } else if (channelPWM <= 450) {
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
    if (channelPWM >= 1500) {
      cam->sendZoomIn();
    } else if (channelPWM >= 800 && channelPWM <= 1100) {
      cam->sendZoomStop();
    } else if (channelPWM <= 450) {
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
    if (channelPWM >= 1500) {
      cam->sendPitchToUp();
    } else if (channelPWM >= 800 && channelPWM <= 1100) {
      cam->sendCenterPitch();
    } else if (channelPWM <= 450) {
      cam->sendPitchToDown();
    }
  } else {
    return false;
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

  logSerial.begin(115200);
  camSerial.begin(115200);
  sbus.begin();

  delay(100);

  for (uint8_t i = 0; i < 16; ++i) channels[i] = 0;

  ZCamProvider::registerDefaults();

  auto* cam = ZCamProvider::createCamera(F("tapotek.KHP290G609"), F("baud=115200;pin=2,3"));

  if (cam) {
    zcam.registerCamera(cam, /*takeOwnership=*/true);
    zcam.setCameraSerial(camSerial);
    zcam.initCamera();
  }

  // Registering event handlers for channels changed values
  zcam.addEventHandler("channel" + String(VIDEO_FORMAT_CHANNEL), "", onVideoFormatChange);
  zcam.addEventHandler("channel" + String(ZOOM_CHANNEL), "", onZoomChange);
  zcam.addEventHandler("channel" + String(PITCH_TOGGLE_CHANNEL), "", onPitchToggleChange);
  zcam.addCombinedEventHandler(pitchChannels, 2, +onPitchXYChange, /*changeThreshold=*/5, "pitchXY");
}

void loop() {
  if (sbus.read(channels, &failsafe, &lostFrame)) {
    zcam.tick(channels);
  }
}
