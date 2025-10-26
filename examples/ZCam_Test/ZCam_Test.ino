#include <Arduino.h>
#include <SoftwareSerial.h>
#include <ZCamProvider.h>
#include "SBUS.h"
#include <Cameras/TapotekCamera.h>
#include <ZCam.h>

// UART к камере (TX - 3 / RX - 2)
#define CAMERA_UART_TX 3
#define CAMERA_UART_RX 2

// Для логов UART (только если будете подключать отдельный UART USB)
#define LOG_TX 6
#define LOG_RX 5

// Количество обрабатываемых каналов, можно уменьшить количество каналов, если не используете больше 8 (например), скорость обработки увеличится
#define TOTAL_CHANNELS 16

#define VIDEO_FORMAT_CHANNEL 4 // Канал для управления видео (режим тепловизора и управление выводимой камерой (окно в окне и т.п.))
#define ZOOM_CHANNEL 0 // Канал для зумирования
#define PITCH_TOGGLE_CHANNEL 3 // Канал для управления PITCH (жесткое задание вверх/по центру/вниз (90 градусов))
#define PITCH_Y_CHANNEL 1 // Для мягкого управления PITCH (аккуратно, можно перенагрузить камеру)
#define PITCH_X_CHANNEL 2 // Для мягкого управления PITCH (аккуратно, можно перенагрузить камеру)

// Для логирования, можно убрать если не используете, главное по коду дальше также удалите использование logSerial
SoftwareSerial logSerial(LOG_TX, LOG_RX);

// Порт для камеры
SoftwareSerial camSerial(CAMERA_UART_TX, CAMERA_UART_RX);

// Инициализация SBUS
SBUS sbus(Serial);

// Инициализация ZCam
ZCam zcam;

// Создание массива с PWM значениями каналов (немного отличаются от реальных, чаще всего от 270 до 1800 примерно)
uint16_t channels[TOTAL_CHANNELS];

// Сборка номеров каналов для мягкого управления PITCH камеры
const uint8_t pitchChannels[2] = { PITCH_X_CHANNEL, PITCH_Y_CHANNEL };

bool failsafe;
bool lostFrame;

enum Move { MOVE_STOP,
            MOVE_UP,
            MOVE_DOWN,
            MOVE_LEFT,
            MOVE_RIGHT };

// Обработчик события управления режимом видео от тепловизора и модами окно в окне, а также переключение между тепловизором и обычной камерой
bool onVideoFormatChange(const String& eventName, uint16_t ch[TOTAL_CHANNELS], const String& condition) {
  uint16_t channelPWM = ch[VIDEO_FORMAT_CHANNEL];
  TapotekCamera* cam = (TapotekCamera*)zcam.camera();

  if (cam) {
    if (channelPWM >= 1500) { // 1500 - это примерно когда стик вверх (тумблер)
      cam->sendChangeIRColorMode();
    } else if (channelPWM <= 450) { // нижнее положение тумблера
      cam->sendChangeWindowMode();
    }
  } else {
    return false;
  }

  return true;
}

// Обработчик отлова события зумирования
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

// Обработчик события жесткого управления положения камеры (три позиции - вверх, по центру и вниз)
bool onPitchToggleChange(const String& eventName, uint16_t ch[TOTAL_CHANNELS], const String& condition) {
  uint16_t channelPWM = ch[PITCH_TOGGLE_CHANNEL];
  TapotekCamera* cam = (TapotekCamera*)zcam.camera();

  if (cam) {
    if (channelPWM >= 1500) { 
      cam->sendPitchToUp(); // камера смотрит вверх
    } else if (channelPWM >= 800 && channelPWM <= 1100) { 
      cam->sendCenterPitch(); // центральное положение
    } else if (channelPWM <= 450) {
      cam->sendPitchToDown(); // камера смотрит ровно вниз (90 градусов)
    }
  } else {
    return false;
  }

  return true;
}

// Обработчик события для мягкого управления PITCH камеры (не советую использовать, находится в разработке)
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

  // Log и порт камеры запускаем на 115200 бод
  logSerial.begin(115200);
  camSerial.begin(115200);
  sbus.begin(); // стартуем SBUS

  delay(100); // приостанавливаем работу на 100 мс

  // Установка стандартного значения для всех каналов в 0
  for (uint8_t i = 0; i < TOTAL_CHANNELS; ++i) channels[i] = 0;

  // Запускаем обработку камер
  ZCamProvider::registerDefaults();

  // Выбираем поддерживаемую камеру (для большенства Tapotek подходит описанная ниже)
  auto* cam = ZCamProvider::createCamera(F("tapotek.KHP290G609"), F("baud=115200;pin=2,3"));

  // Если камеру удалось запустить (внутренняя логика, не реальная камера), то регистрируем ее внутри сервиса и задаем ей порт
  if (cam) {
    zcam.registerCamera(cam, /*takeOwnership=*/true);
    zcam.setCameraSerial(camSerial);
    zcam.initCamera();
  }

  // Регистрируем обработчики событий на конкретные события
  // СОВЕТ: При первом использовании включать только один обработчик и по одному подключать последующие
  zcam.addEventHandler("channel" + String(VIDEO_FORMAT_CHANNEL), "", onVideoFormatChange); // Ловим канал для управления режимами видео (тепловизор и выбор камеры)
  zcam.addEventHandler("channel" + String(ZOOM_CHANNEL), "", onZoomChange); // Ловим канал управления зумированием
  zcam.addEventHandler("channel" + String(PITCH_TOGGLE_CHANNEL), "", onPitchToggleChange); // Ловим жесткое управление позицией PITCH камеры
  zcam.addCombinedEventHandler(pitchChannels, 2, +onPitchXYChange, /*changeThreshold=*/5, "pitchXY"); // Комбинированный метод захвата событий сразу на нескольких каналах для работы мягкого PITCH
}

void loop() {

  // Если SBUS доступен и отдает ответ, то запускаем обработку ZCam и передаем туда обновленные PWM значения каналов
  if (sbus.read(channels, &failsafe, &lostFrame)) {
    zcam.tick(channels);
  }
}
