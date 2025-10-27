# 📷 ZCam - добавление новой камеры в ZCam

Этот документ описывает устройство подсистемы камер в библиотеке **ZCam**: что такое **провайдер**, зачем нужен **базовый интерфейс**, как работает **проксирование** в ядре `ZCam`, и как создать **новую камеру** — как UART‑вариант, так и полностью GPIO‑вариант (без UART).

---

## 🧠 Архитектура камер в двух словах

- **Интерфейс камеры** (`ZCamCameraBaseInterface`) — контракт, который обязана реализовать любая камера.
- **База камеры** (`ZCamCameraBase`) — удобная реализация по умолчанию (часть методов уже реализованы).
- **Провайдер** (`ZCamProvider`) — фабрика/реестр, создаёт камеры по строковому ключу (например, `"tapotek.KHP290G609"`).
- **Ядро** (`ZCam`) — хранит текущую камеру, проксирует к ней полезные методы (инициализация, центрирование и т.п.) и передаёт её в события.

Схематично:

```
ZCam  --(registerCamera)-->  ZCamCameraBaseInterface (полиморфизм)
   ^                                 ↑
   |                                 |
   +-- ZCamProvider::createCamera ---+
```

---

## 🧩 Что такое провайдер и как он работает

**ZCamProvider** — статический реестр: строковый ключ → функция‑создатель камеры.

- **Регистрация** (обычно один раз в `setup()` или внутри `registerDefaults()`):
  ```cpp
  ZCamProvider::registerCameraType("tapotek.KHP290G609",
    [](const String& params)->ZCamCameraBaseInterface* {
      auto* cam = new TapotekKHP290G609Camera();
      if (cam) cam->init(params);   // необязательно: можно отложить init
      return cam;
    });
  ```

- **Создание**:
  ```cpp
  auto* cam = ZCamProvider::createCamera("tapotek.KHP290G609", "baud=115200;pin=2,3");
  ```

- **Параметры** — произвольная строка (`"ключ=значение;ключ2=значение2"`), парсится внутри класса камеры.

Плюсы такого подхода: ядро **не знает** о конкретных моделях — добавление новых камер не ломает `ZCam` и клиентский код.

---

## 🧾 Обязательные методы камеры (интерфейс)

Любая камера должна реализовать `ZCamCameraBaseInterface`:

```cpp
struct ZCamCameraBaseInterface {
  virtual ~ZCamCameraBaseInterface() {}
  virtual bool init() = 0;
  virtual bool init(const String& params) = 0;     // можно делегировать на init()
  virtual void setSerial(SoftwareSerial& ser) = 0; // если не нужен UART — оставить пустым телом
  virtual bool centerPosition() = 0;
  virtual ZCamDataPosition getPosition() const = 0;
};
```

Рекомендуется наследоваться от **`ZCamCameraBase`** — в нём уже есть:
- дефолтная реализация `init(const String&)` (делегирует на `init()`),
- хранение `ZCamData`,
- заглушки `centerPosition()` и т.п.,
- поле `_serial` (если нужен UART).

Почему **нужно наследовать интерфейс** (а лучше `ZCamCameraBase`):
- единый **контракт** — ядро и события работают со всеми камерами одинаково;
- **полиморфизм** — легко подменять тип камеры без изменения пользовательского кода;
- **инкапсуляция** — детали реализации камеры скрыты внутри класса.

---

## 🧷 Проксирование в `ZCam`

`ZCam` хранит указатель на активную камеру и предоставляет прокси‑методы:
- `registerCamera(cam, takeOwnership)` — зарегистрировать камеру;
- `initCamera()` — вызывает `camera->init()`;
- `centerCamera()` — вызывает `camera->centerPosition()`;
- `getCameraPos()` — проксирует `camera->getPosition()`;
- `setCameraSerial(SoftwareSerial&)` — пробрасывает в `camera->setSerial(...)`.

События (`ZCamEvent*`) получают указатель на камеру через `attachCamera()` и могут вызывать её методы напрямую.

---

## 🚀 Как добавить новую камеру: пошагово

1. **Создайте файлы** в `src/Cameras/`:
   - `MyCamera.h`
   - `MyCamera.cpp`

2. **Наследуйтесь** от `ZCamCameraBase` или `ZCamCameraBaseInterface`.

3. **Реализуйте** обязательные методы (`init()`, `centerPosition()`, `getPosition()`) и свои команды.

4. **Зарегистрируйте** камеру в провайдере (в `ZCamProvider::registerDefaults()` или прямо в `setup()` пользователя).

5. **Используйте** в скетче:
   ```cpp
   auto* cam = ZCamProvider::createCamera("my.camera", "custom=params");
   zcam.registerCamera(cam, true);
   zcam.initCamera();
   ```

---

## 🧪 UART‑камера (схема как у Tapotek)

Ниже упрощённый шаблон (без конкретных протокольных команд):

```cpp
// src/Cameras/MyUartCamera.h
#pragma once
#include "ZCamCameraBase.h"

class MyUartCamera : public ZCamCameraBase {
public:
  using ZCamCameraBase::init;
  bool init() override;
  bool init(const String& params) override;

  bool centerPosition() override;
  ZCamDataPosition getPosition() const override { return _data.position; }

  // Пример своих команд:
  bool zoomIn();
  bool zoomOut();
};
```

```cpp
// src/Cameras/MyUartCamera.cpp
#include "MyUartCamera.h"
#include <SoftwareSerial.h>

static int extractInt(const String& src, const String& key, int defVal) {
  int start = src.indexOf(key + "=");
  if (start < 0) return defVal;
  start += key.length() + 1;
  int end = src.indexOf(';', start);
  String val = (end < 0) ? src.substring(start) : src.substring(start, end);
  val.trim();
  return val.length() ? val.toInt() : defVal;
}

bool MyUartCamera::init() {
  // базовая инициализация состояния
  _data.position.x = _data.position.y = _data.position.z = 0;
  // можно послать стартовую команду через _serial, если задан
  // if (_serial) { _serial->write(...); }
  return true;
}

bool MyUartCamera::init(const String& params) {
  // пример: "baud=9600"
  int baud = extractInt(params, F("baud"), 115200);
  // настройка скорости происходит в скетче через camSerial.begin(baud)
  // здесь просто запоминаем/проверяем
  (void)baud;
  return init();
}

bool MyUartCamera::centerPosition() {
  // отправка команды центрирования
  // if (_serial) { _serial->write(...); }
  return true;
}

bool MyUartCamera::zoomIn()  { /* if (_serial) _serial->write(...); */ return true; }
bool MyUartCamera::zoomOut() { /* if (_serial) _serial->write(...); */ return true; }
```

**Регистрация в провайдере:**
```cpp
// в ZCamProvider::registerDefaults()
ZCamProvider::registerCameraType("my.camera",
  [](const String& params)->ZCamCameraBaseInterface* {
    auto* cam = new MyUartCamera();
    if (cam) cam->init(params);
    return cam;
  });
```

---

## 🔌 GPIO‑камера (без UART): управление напрямую с Arduino пинов

Пример камеры, у которой команды подаются **напрямую** на пины Arduino:
- D4 — поворот вверх
- D5 — поворот вниз
- D6 — поворот влево
- D7 — поворот вправо
- D8 — зум +
- D9 — зум −

Мы просто выставляем уровни на соответствующих выводах. UART не нужен.

```cpp
// src/Cameras/GpioGimbalCamera.h
#pragma once
#include "ZCamCameraBase.h"

class GpioGimbalCamera : public ZCamCameraBase {
public:
  using ZCamCameraBase::init;
  bool init() override;
  bool init(const String& params) override;

  // базовые операции PTZ
  bool centerPosition() override; // можно реализовать как остановку по всем осям
  ZCamDataPosition getPosition() const override { return _data.position; }

  // команды движения
  bool moveUp();
  bool moveDown();
  bool moveLeft();
  bool moveRight();
  bool stopMove();

  // зум
  bool zoomIn();
  bool zoomOut();
  bool zoomStop();

private:
  // Пины по умолчанию
  uint8_t _pinUp    = 4;
  uint8_t _pinDown  = 5;
  uint8_t _pinLeft  = 6;
  uint8_t _pinRight = 7;
  uint8_t _pinZp    = 8; // zoom+
  uint8_t _pinZm    = 9; // zoom-

  void allLow();
  static int extractInt(const String& src, const String& key, int defVal);
};
```

```cpp
// src/Cameras/GpioGimbalCamera.cpp
#include "GpioGimbalCamera.h"
#include <Arduino.h>

int GpioGimbalCamera::extractInt(const String& src, const String& key, int defVal) {
  int start = src.indexOf(key + "=");
  if (start < 0) return defVal;
  start += key.length() + 1;
  int end = src.indexOf(';', start);
  String val = (end < 0) ? src.substring(start) : src.substring(start, end);
  val.trim();
  return val.length() ? val.toInt() : defVal;
}

bool GpioGimbalCamera::init() {
  pinMode(_pinUp,    OUTPUT);
  pinMode(_pinDown,  OUTPUT);
  pinMode(_pinLeft,  OUTPUT);
  pinMode(_pinRight, OUTPUT);
  pinMode(_pinZp,    OUTPUT);
  pinMode(_pinZm,    OUTPUT);
  allLow();
  return true;
}

bool GpioGimbalCamera::init(const String& params) {
  // Пример параметров: "pins=4,5,6,7,8,9"
  int start = params.indexOf(F("pins="));
  if (start >= 0) {
    start += 5;
    int end = params.indexOf(';', start);
    String list = (end < 0) ? params.substring(start) : params.substring(start, end);
    list.trim();
    // ожидаем 6 чисел через запятую
    int vals[6] = {-1,-1,-1,-1,-1,-1};
    int i = 0;
    while (list.length() && i < 6) {
      int c = list.indexOf(',');
      String tok = (c >= 0) ? list.substring(0,c) : list;
      tok.trim();
      vals[i++] = tok.toInt();
      if (c < 0) break;
      list.remove(0, c+1);
    }
    if (i == 6) {
      _pinUp    = (uint8_t)vals[0];
      _pinDown  = (uint8_t)vals[1];
      _pinLeft  = (uint8_t)vals[2];
      _pinRight = (uint8_t)vals[3];
      _pinZp    = (uint8_t)vals[4];
      _pinZm    = (uint8_t)vals[5];
    }
  }
  return init();
}

void GpioGimbalCamera::allLow() {
  digitalWrite(_pinUp,    LOW);
  digitalWrite(_pinDown,  LOW);
  digitalWrite(_pinLeft,  LOW);
  digitalWrite(_pinRight, LOW);
  digitalWrite(_pinZp,    LOW);
  digitalWrite(_pinZm,    LOW);
}

// ====== PTZ движения ======
bool GpioGimbalCamera::moveUp()    { allLow(); digitalWrite(_pinUp,    HIGH); return true; }
bool GpioGimbalCamera::moveDown()  { allLow(); digitalWrite(_pinDown,  HIGH); return true; }
bool GpioGimbalCamera::moveLeft()  { allLow(); digitalWrite(_pinLeft,  HIGH); return true; }
bool GpioGimbalCamera::moveRight() { allLow(); digitalWrite(_pinRight, HIGH); return true; }
bool GpioGimbalCamera::stopMove()  { allLow(); return true; }

// ====== Зум ======
bool GpioGimbalCamera::zoomIn()    { digitalWrite(_pinZm, LOW); digitalWrite(_pinZp, HIGH); return true; }
bool GpioGimbalCamera::zoomOut()   { digitalWrite(_pinZp, LOW); digitalWrite(_pinZm, HIGH); return true; }
bool GpioGimbalCamera::zoomStop()  { digitalWrite(_pinZp, LOW); digitalWrite(_pinZm, LOW);  return true; }

bool GpioGimbalCamera::centerPosition() {
  // Для GPIO‑варианта определим как остановку по всем направлениям
  return stopMove();
}
```

**Регистрация в провайдере:**
```cpp
// в ZCamProvider::registerDefaults()
ZCamProvider::registerCameraType("gpio.gimbal",
  [](const String& params)->ZCamCameraBaseInterface* {
    auto* cam = new GpioGimbalCamera();
    if (cam) cam->init(params); // params может быть "pins=4,5,6,7,8,9"
    return cam;
  });
```

**Использование в скетче:**
```cpp
auto* cam = ZCamProvider::createCamera("gpio.gimbal", "pins=4,5,6,7,8,9");
zcam.registerCamera(cam, true);
zcam.initCamera();
```

---

## ✅ Рекомендации по дизайну классов камер

- Не делайте длительных задержек (`delay`) внутри методов камеры — лучше таймить действия в `loop()`/событиях.
- Обрабатывайте отсутствие ресурсов аккуратно: если `_serial == nullptr` — возвращайте `false`/ничего не делайте.
- Логи добавляйте условно (чтобы пользователю не пришлось плодить `Serial.println` в релизе).
- Для параметров используйте ключ=значение; храните дефолты внутри класса.

---

## 🧪 Тест‑чеклист

- [ ] Камера создаётся через `ZCamProvider::createCamera(...)` и корректно инициализируется.
- [ ] Команды вызываются из обработчиков событий без зависаний.
- [ ] При смене камеры код в `.ino` не меняется (полиморфизм работает).
- [ ] Параметры (`baud`, `pins`, etc.) корректно разбираются.

---

Готово! Теперь вы можете добавлять новые модели камер и типы подключений, не меняя код ядра ZCam и не ломая пользовательские скетчи. Удачных проектов 🚀
