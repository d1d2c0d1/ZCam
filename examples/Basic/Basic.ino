#include <ZCam.h>

ZCam zcam;
static uint16_t channels[16];

bool cbFunction(const String& eventName, uint16_t ch[16], const String& condition) {
  Serial.print(F("[CB] ")); Serial.print(eventName);
  Serial.print(F(" cond=")); Serial.print(condition);
  Serial.print(F(" ch0="));  Serial.println(ch[0]);

  // Пример: при срабатывании — центрируем камеру
  zcam.centerCamera(); // безопасно: вернёт false, если камера не установлена
  return true;
}

void setup() {
  Serial.begin(115200);
  for (uint8_t i=0;i<16;i++) channels[i]=1500;

  // Зарегистрируем камеру (ядро владеет и удалит при деструкторе)
  auto* cam = new TapotekCamera();
  zcam.registerCamera(cam, /*takeOwnership=*/true);
  zcam.initCamera();

  // Событие по channel0
  zcam.addEventHandler(F("channel0"), F(">1200&<=1500&!1350"), cbFunction);
}

void loop() {
  // Здесь — реальные данные SBUS → channels[0..15] (1000..2000)
  // ...

  zcam.tick(channels);
  delay(5);
}
