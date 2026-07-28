// Copyright (c) 2025 Arduino SA
// SPDX-License-Identifier: MPL-2.0

#ifndef ARDUINO_LIBRARIES_MODULINO_H
#define ARDUINO_LIBRARIES_MODULINO_H

#if defined(ESP32) && defined(BOARD_HAS_PIN_REMAP) && defined(tone)
  #error "The current configuration is unsupported, switch Pin Numbering to "By GPIO number" or #undef tone and #undef noTone in the beginning of your sketch."
  #error "Learn more at: https://support.arduino.cc/hc/en-us/articles/10483225565980-Select-pin-numbering-for-Nano-ESP32-in-Arduino-IDE"
#endif

#include "Wire.h"
#include <vl53l4cd_class.h>  // from stm32duino
#include <vl53l4ed_class.h>  // from stm32duino
#include "Arduino_LSM6DSOX.h"
#include <Arduino_LPS22HB.h>
#include <Arduino_HS300x.h>
#include "Arduino_LTR381RGB.h"
#include "Arduino.h"
//#include <SE05X.h>  // need to provide a way to change Wire object

#ifndef ARDUINO_API_VERSION
#define PinStatus     uint8_t
#define HardwareI2C   TwoWire
#endif

typedef enum {
  STOP = 0,
  GENTLE = 25,
  MODERATE = 30,
  MEDIUM = 35,
  INTENSE = 40,
  POWERFUL = 45,
  MAXIMUM = 50
} VibroPowerLevel;

void __increaseI2CPriority();

class ModulinoClass {
public:
#if defined(ARDUINO_UNOR4_WIFI) || defined(ARDUINO_NANO_R4) || defined(ARDUINO_UNO_Q)
  void begin(HardwareI2C& wire = Wire1) {
#else
  void begin(HardwareI2C& wire = Wire) {
#endif

#ifdef ARDUINO_UNOR4_WIFI
    // unlock Wire1 bus at begin since we don't know the state of the system
    pinMode(WIRE1_SCL_PIN, OUTPUT);
    for (int i = 0; i < 20; i++) {
      digitalWrite(WIRE1_SCL_PIN, HIGH);
      digitalWrite(WIRE1_SCL_PIN, LOW);
    }
#endif
    _wire = &wire;
    _wire->begin();
    _wire->setClock(100000);
    __increaseI2CPriority();
  }
  friend class Module;
protected:
  HardwareI2C* _wire;
  friend class ModulinoHub;
  friend class ModulinoHubPort;
};

extern ModulinoClass Modulino;

// Forward declaration of ModulinoHub
class ModulinoHub;

class ModulinoHubPort {
  public:
    ModulinoHubPort(int port, ModulinoHub* hub) : _port(port), _hub(hub) {}
    int select();
    int clear();
  private:
    int _port;
    ModulinoHub* _hub;
};

class ModulinoHub {
  public:
    ModulinoHub(int address = 0x70) : _address(address){  }
    ModulinoHubPort* port(int _port) {
      return new ModulinoHubPort(_port, this);
    }
    int select(int port) {
      Modulino._wire->beginTransmission(_address);
      Modulino._wire->write(1 << port);
      return Modulino._wire->endTransmission();
    }
    int clear() {
      Modulino._wire->beginTransmission(_address);
      Modulino._wire->write((uint8_t)0);
      return Modulino._wire->endTransmission();
    }

    int address() {
      return _address;
    }
  private:
    int _address;
};

class Module : public Printable {
public:
  Module(uint8_t address = 0xFF, const char* name = "", ModulinoHubPort* hubPort = nullptr)
    : address(address), name((char *)name), hubPort(hubPort) {}
  virtual ~Module() {}  
  bool begin() {
    if (address >= 0x7F) {
      address = discover() / 2;  // divide by 2 to match address in fw main.c
    }
    return (address < 0x7F);
  }
  virtual uint8_t discover() {
    return 0xFF;
  }
  operator bool() {
    return address < 0x7F;
  }
  static HardwareI2C* getWire() {
    return Modulino._wire;
  }
  bool read(uint8_t* buf, int howmany) {
    if (address >= 0x7F) {
      return false;
    }
    if (hubPort != nullptr) {
      hubPort->select();
    }
    Modulino._wire->requestFrom(address, howmany + 1);
    auto start = millis();
    while ((Modulino._wire->available() == 0) && (millis() - start < 100)) {
      delay(1);
    }
    if (Modulino._wire->available() < howmany) {
      return false;
    }
    pinstrap_address = Modulino._wire->read();
    for (int i = 0; i < howmany; i++) {
      buf[i] = Modulino._wire->read();
    }
    while (Modulino._wire->available()) {
      Modulino._wire->read();
    }
    if (hubPort != nullptr) {
      hubPort->clear();
    }
    return true;
  }
  bool write(uint8_t* buf, int howmany) {
    if (address >= 0x7F) {
      return false;
    }
    if (hubPort != nullptr) {
      hubPort->select();
    }
    Modulino._wire->beginTransmission(address);
    for (int i = 0; i < howmany; i++) {
      Modulino._wire->write(buf[i]);
    }
    Modulino._wire->endTransmission();
    if (hubPort != nullptr) {
      hubPort->clear();
    }
    return true;
  }
  bool nonDefaultAddress() {
    return (pinstrap_address != address);
  }
  virtual size_t printTo(Print& p) const {
    return p.print(name);
  }
  bool scan(uint8_t addr) {
    if (hubPort != nullptr) {
      hubPort->select();
    }
    Modulino._wire->beginTransmission(addr / 2);  // multply by 2 to match address in fw main.c
    auto ret = Modulino._wire->endTransmission();
    if (hubPort != nullptr) {
      hubPort->clear();
    }
    if (ret == 0) {
      // could also ask for 1 byte and check if it's truely a modulino of that kind
      return true;
    }
    return false;
  }
private:
  uint8_t address;
  uint8_t pinstrap_address;
  char* name;
protected:
  ModulinoHubPort* hubPort = nullptr;
  /**
   * @brief The resolved 7-bit I2C address of this module.
   * For modules whose reply length or layout varies with an operating mode,
   * and which therefore cannot use the fixed-size read() helper above.
   */
  uint8_t getAddress() const {
    return address;
  }
};

class ModulinoButtons : public Module {
public:
  ModulinoButtons(uint8_t address = 0xFF, ModulinoHubPort* hubPort = nullptr)
    : Module(address, "BUTTONS", hubPort) {}
  ModulinoButtons(ModulinoHubPort* hubPort, uint8_t address = 0xFF)
    : Module(address, "BUTTONS", hubPort) {}
  PinStatus isPressed(int index) {
    return last_status[index] ? HIGH : LOW;
  }
  PinStatus isPressed(char button) {
    int index = buttonToIndex(button);
    if (index < 0) return LOW;
    return isPressed(index);
  }
  PinStatus isPressed(const char *button) {
    if (button == nullptr || button[0] == '\0' || button[1] != '\0') {
      return LOW;
    }
    return isPressed(button[0]);
  }
  bool update() {
    uint8_t buf[3];
    auto res = read((uint8_t*)buf, 3);
    auto ret = res && (buf[0] != last_status[0] || buf[1] != last_status[1] || buf[2] != last_status[2]);
    last_status[0] = buf[0];
    last_status[1] = buf[1];
    last_status[2] = buf[2];
    return ret;
  }
  void setLeds(bool a, bool b, bool c) {
    uint8_t buf[3];
    buf[0] = a;
    buf[1] = b;
    buf[2] = c;
    write((uint8_t*)buf, 3);
    return;
  }
  virtual uint8_t discover() {
    for (unsigned int i = 0; i < sizeof(match)/sizeof(match[0]); i++) {
      if (scan(match[i])) {
        return match[i];
      }
    }
    return 0xFF;
  }
private:
  bool last_status[3];
  int buttonToIndex(char button) {
    switch (toupper(button)) {
      case 'A': return 0;
      case 'B': return 1;
      case 'C': return 2;
      default:  return -1;
    }
  }
protected:
  uint8_t match[1] = { 0x7C };  // same as fw main.c
};

class ModulinoJoystick : public Module {
public:
  ModulinoJoystick(uint8_t address = 0xFF, ModulinoHubPort* hubPort = nullptr)
    : Module(address, "JOYSTICK", hubPort) {}
  ModulinoJoystick(ModulinoHubPort* hubPort, uint8_t address = 0xFF)
    : Module(address, "JOYSTICK", hubPort) {}
  bool update() {
    uint8_t buf[3];
    auto res = read((uint8_t*)buf, 3);
    auto x = buf[0];
    auto y =  buf[1];
    map_value(x, y);
    auto ret = res && (x != last_status[0] || y != last_status[1] || buf[2] != last_status[2]);
    if (!ret) {
      return false;
    }
    last_status[0] = x;
    last_status[1] = y;
    last_status[2] = buf[2];
    return ret;
  }
  void setDeadZone(uint8_t dz_th) {
    _dz_threshold = dz_th;
  }
  PinStatus isPressed() {
    return last_status[2] ? HIGH : LOW;
  }
  int8_t getX() {
    return (last_status[0] < 128 ? (128 - last_status[0]) : -(last_status[0] - 128));
  }
  int8_t getY() {
    return (last_status[1] < 128 ? (128 - last_status[1]) : -(last_status[1] - 128));
  }
  virtual uint8_t discover() {
    for (unsigned int i = 0; i < sizeof(match)/sizeof(match[0]); i++) {
      if (scan(match[i])) {
        return match[i];
      }
    }
    return 0xFF;
  }
  void map_value(uint8_t &x, uint8_t &y) {
    if (x > 128 - _dz_threshold &&  x < 128 + _dz_threshold && y > 128 - _dz_threshold && y < 128 + _dz_threshold) {
        x = 128;
        y = 128;
    }
  }
private:
  uint8_t _dz_threshold = 26;
  uint8_t last_status[3];
protected:
  uint8_t match[1] = { 0x58 };  // same as fw main.c
};

class ModulinoBuzzer : public Module {
public:
  ModulinoBuzzer(uint8_t address = 0xFF, ModulinoHubPort* hubPort = nullptr)
    : Module(address, "BUZZER", hubPort) {}
  ModulinoBuzzer(ModulinoHubPort* hubPort, uint8_t address = 0xFF)
    : Module(address, "BUZZER", hubPort) {}
  void (tone)(size_t freq, size_t len_ms) {
    uint8_t buf[8];
    memcpy(&buf[0], &freq, 4);
    memcpy(&buf[4], &len_ms, 4);
    write(buf, 8);
  }
  void (noTone)() {
    uint8_t buf[8];
    memset(&buf[0], 0, 8);
    write(buf, 8);
  }
  virtual uint8_t discover() {
    for (unsigned int i = 0; i < sizeof(match)/sizeof(match[0]); i++) {
      if (scan(match[i])) {
        return match[i];
      }
    }
    return 0xFF;
  }
protected:
  uint8_t match[1] = { 0x3C };  // same as fw main.c
};

class ModulinoVibro : public Module {
public:
  ModulinoVibro(uint8_t address = 0xFF, ModulinoHubPort* hubPort = nullptr)
    : Module(address, "VIBRO", hubPort) {}
  ModulinoVibro(ModulinoHubPort* hubPort, uint8_t address = 0xFF)
    : Module(address, "VIBRO", hubPort) {}
  void on(size_t len_ms, bool block, int power = MAXIMUM ) {
    uint8_t buf[12];
    uint32_t freq = 1000;
    memcpy(&buf[0], &freq, 4);
    memcpy(&buf[4], &len_ms, 4);
    memcpy(&buf[8], &power, 4);
    write(buf, 12);
    if (block) {
      delay(len_ms);
      off();
    }
  }
  void on(size_t len_ms) {
    on(len_ms, false);
  }
  void on(size_t len_ms, VibroPowerLevel power) {
    on(len_ms, false, power);
  }
  void off() {
    uint8_t buf[8];
    memset(&buf[0], 0, 8);
    write(buf, 8);
  }
  virtual uint8_t discover() {
    for (unsigned int i = 0; i < sizeof(match)/sizeof(match[0]); i++) {
      if (scan(match[i])) {
        return match[i];
      }
    }
    return 0xFF;
  }
protected:
  uint8_t match[1] = { 0x70 };  // same as fw main.c
};


class ModulinoColor {
public:
  ModulinoColor(uint8_t r, uint8_t g, uint8_t b)
    : r(r), g(g), b(b) {}
  operator uint32_t() {
    return (b << 8 | g << 16 | r << 24);
  }
private:
  uint8_t r, g, b;
};

class ModulinoPixels : public Module {
public:
  ModulinoPixels(uint8_t address = 0xFF, ModulinoHubPort* hubPort = nullptr)
    : Module(address, "LEDS", hubPort) {
    memset((uint8_t*)data, 0xE0, NUMLEDS * 4);
  }
  ModulinoPixels(ModulinoHubPort* hubPort, uint8_t address = 0xFF)
    : Module(address, "LEDS", hubPort) {
    memset((uint8_t*)data, 0xE0, NUMLEDS * 4);
  }
  void set(int idx, ModulinoColor rgb, uint8_t brightness = 25) {
    if (idx < NUMLEDS) {
      uint8_t _brightness = map(brightness, 0, 100, 0, 0x1F);
      data[idx] = (uint32_t)rgb | _brightness | 0xE0;
    }
  }
  void set(int idx, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness = 5) {
    set(idx, ModulinoColor(r,g,b), brightness);
  }
  void clear(int idx) {
    set(idx, ModulinoColor(0,0,0), 0);
  }
  void clear() {
    memset((uint8_t*)data, 0xE0, NUMLEDS * 4);
  }
  void show() {
    write((uint8_t*)data, NUMLEDS * 4);
  }
  virtual uint8_t discover() {
    for (unsigned int i = 0; i < sizeof(match)/sizeof(match[0]); i++) {
      if (scan(match[i])) {
        return match[i];
      }
    }
    return 0xFF;
  }
private:
  static const int NUMLEDS = 8;
  uint32_t data[NUMLEDS];
protected:
  uint8_t match[1] = { 0x6C };
};


class ModulinoKnob : public Module {
public:
  ModulinoKnob(uint8_t address = 0xFF, ModulinoHubPort* hubPort = nullptr)
    : Module(address, "ENCODER", hubPort) {}
  ModulinoKnob(ModulinoHubPort* hubPort, uint8_t address = 0xFF)
    : Module(address, "ENCODER", hubPort) {}
    bool begin() {
    auto ret = Module::begin();
    if (ret) {
      auto _val = get();
      _lastPosition = _val;
      _lastDebounceTime = millis();
      set(100);
      if (get() != 100) {
        _bug_on_set = true;
        set(-_val);
      } else {
        set(_val);
      }
    }
    return ret;
  }
  int16_t get() {
    uint8_t buf[3];
    auto res = read(buf, 3);
    if (res == false) {
      return 0;
    }
    _pressed = (buf[2] != 0);
    int16_t ret = buf[0] | (buf[1] << 8);
    return ret;
  }
  void set(int16_t value) {
    if (_bug_on_set) {
      value = -value;
    }
    uint8_t buf[4];
    memcpy(buf, &value, 2);
    write(buf, 4);
  }
  bool isPressed() {
    get();
    return _pressed;
  }
  int8_t getDirection() {
    unsigned long now = millis();
    if (now - _lastDebounceTime < DEBOUNCE_DELAY) {
      return 0;
    }
    int16_t current = get();
    int8_t direction = 0;
    if (current > _lastPosition) {
      direction = 1;
    } else if (current < _lastPosition) {
      direction = -1;
    }
    if (direction != 0) {
      _lastDebounceTime = now;
      _lastPosition = current;
    }
    return direction;
  }
  virtual uint8_t discover() {
    for (unsigned int i = 0; i < sizeof(match)/sizeof(match[0]); i++) {
      if (scan(match[i])) {
        return match[i];
      }
    }
    return 0xFF;
  }
private:
  bool _pressed = false;
  bool _bug_on_set = false;
  int16_t _lastPosition = 0;
  unsigned long _lastDebounceTime = 0;
  static constexpr unsigned long DEBOUNCE_DELAY = 30;
protected:
  uint8_t match[2] = { 0x74, 0x76 };
};

extern ModulinoColor BLACK;
extern ModulinoColor RED;
extern ModulinoColor BLUE;
extern ModulinoColor GREEN;
extern ModulinoColor YELLOW;
extern ModulinoColor VIOLET;
extern ModulinoColor CYAN;
extern ModulinoColor WHITE;

class ModulinoMovement : public Module {
public:
  ModulinoMovement(ModulinoHubPort* hubPort = nullptr)
    : Module(0xFF, "MOVEMENT", hubPort) {}
  bool begin() {
    if (hubPort != nullptr) {
      hubPort->select();
    }
    if (_imu == nullptr) {
      _imu = new LSM6DSOXClass(*((TwoWire*)getWire()), 0x6A);
    }
    initialized = _imu->begin();
    __increaseI2CPriority();
    if (hubPort != nullptr) {
      hubPort->clear();
    }
    return initialized != 0;
  }
  operator bool() {
    return (initialized != 0);
  }
  int update() {
    if (initialized) {
      if (hubPort != nullptr) {
        hubPort->select();
      }
      int accel = _imu->readAcceleration(x, y, z);
      int gyro = _imu->readGyroscope(roll, pitch, yaw);
      if (hubPort != nullptr) {
        hubPort->clear();
      }
      return accel && gyro;
    }
    return 0;
  }
  int available() {
    if (initialized) {
      if (hubPort != nullptr) {
        hubPort->select();
      }
      auto ret = _imu->accelerationAvailable() && _imu->gyroscopeAvailable();
      if (hubPort != nullptr) {
        hubPort->clear();
      }
      return ret;
    }
    return 0;
  }
  float getX() {
    return x;
  }
  float getY() {
    return y;
  }
  float getZ() {
    return z;
  }
  float getRoll() {
    return roll;
  }
  float getPitch() {
    return pitch;
  }
  float getYaw() {
    return yaw;
  }
private:
  LSM6DSOXClass* _imu = nullptr;
  float x,y,z;
  float roll,pitch,yaw; //gx, gy, gz
  int initialized = 0;
};

class ModulinoThermo: public Module {
public:
  ModulinoThermo(ModulinoHubPort* hubPort = nullptr)
  : Module(0xFF, "THERMO", hubPort) {}
  bool begin() {
    if (hubPort != nullptr) {
      hubPort->select();
    }
    if (_sensor == nullptr) {
      _sensor = new HS300xClass(*((TwoWire*)getWire()));
    }
    initialized = _sensor->begin();
    __increaseI2CPriority();
    if (hubPort != nullptr) {
      hubPort->clear();
    }
    return initialized;
  }
  operator bool() {
    return (initialized != 0);
  }
  float getHumidity() {
    if (initialized) {
      if (hubPort != nullptr) {
        hubPort->select();
      }
      auto ret = _sensor->readHumidity();
      if (hubPort != nullptr) {
        hubPort->clear();
      }
      return ret;
    }
    return 0;
  }
  float getTemperature() {
    if (initialized) {
      if (hubPort != nullptr) {
        hubPort->select();
      }
      auto ret = _sensor->readTemperature();
      if (hubPort != nullptr) {
        hubPort->clear();
      }
      return ret;
    }
    return 0;
  }
private:
  HS300xClass* _sensor = nullptr;
  int initialized = 0;
};

class ModulinoPressure : public Module {
public:
  ModulinoPressure(ModulinoHubPort* hubPort = nullptr)
    : Module(0xFF, "PRESSURE", hubPort) {}
  bool begin() {
    if (hubPort != nullptr) {
      hubPort->select();
    }
    if (_barometer == nullptr) {
      _barometer = new LPS22HBClass(*((TwoWire*)getWire()));
    }
    initialized = _barometer->begin();
    if (initialized == 0) {
      // unfortunately LPS22HBClass calles Wire.end() on failure, restart it
      getWire()->begin();
    }
    __increaseI2CPriority();
    if (hubPort != nullptr) {
      hubPort->clear();
    }
    return initialized != 0;
  }
  operator bool() {
    return (initialized != 0);
  }
  float getPressure() {
    if (initialized) {
      if (hubPort != nullptr) {
        hubPort->select();
      }
      auto ret = _barometer->readPressure();
      if (hubPort != nullptr) {
        hubPort->clear();
      }
      return ret;
    }
    return 0;
  }
  float getTemperature() {
    if (initialized) {
      if (hubPort != nullptr) {
        hubPort->select();
      }
      auto ret = _barometer->readTemperature();
      if (hubPort != nullptr) {
        hubPort->clear();
      }
      return ret;
    }
    return 0;
  }
private:
  LPS22HBClass* _barometer = nullptr;
  int initialized = 0;
};

class ModulinoLight : public Module {
public:
  ModulinoLight(ModulinoHubPort* hubPort = nullptr)
    : Module(0xFF, "LIGHT", hubPort) {}
  bool begin() {
    if (hubPort != nullptr) {
      hubPort->select();
    }
    if (_light == nullptr) {
      _light = new LTR381RGBClass(*((TwoWire*)getWire()), 0x53);
    }
    initialized = _light->begin();
    __increaseI2CPriority();
    if (hubPort != nullptr) {
      hubPort->clear();
    }
    return initialized != 0;
  }
  operator bool() {
    return (initialized != 0);
  }
  bool update() {
    if (initialized) {
      if (hubPort != nullptr) {
        hubPort->select();
      }
      auto ret = _light->readAllSensors(r, g, b, rawlux, lux, ir);
      if (hubPort != nullptr) {
        hubPort->clear();
      }
    }
    return 0;
  }
  ModulinoColor getColor() {
    return ModulinoColor(r, g, b);
  }
  String getColorApproximate() {
    String color = "UNKNOWN";
    float h, s, l;
    _light->getHSL(r, g, b, h, s, l);

    if (l > 90.0) {
        return "WHITE";
    }
    if (l <= 0.20) {
        return "BLACK";
    }
    if (s < 10.0) {
        if (l < 50.0) {
            return "DARK GRAY";
        } else {
            return "LIGHT GRAY";
        }
    }

    if (h < 0) {
        h = 360 + h;
    }
    if (h < 15 || h >= 345) {
        color = "RED";
    } else if (h < 45) {
        color = "ORANGE";
    } else if (h < 75) {
        color = "YELLOW";
    } else if (h < 105) {
        color = "LIME";
    } else if (h < 135) {
        color = "GREEN";
    } else if (h < 165) {
        color = "SPRING GREEN";
    } else if (h < 195) {
        color = "CYAN";
    } else if (h < 225) {
        color = "AZURE";
    } else if (h < 255) {
        color = "BLUE";
    } else if (h < 285) {
        color = "VIOLET";
    } else if (h < 315) {
        color = "MAGENTA";
    } else {
        color = "ROSE";
    }

    // Adjust color based on lightness
    if (l < 20.0) {
        color = "VERY DARK " + color;
    } else if (l < 40.0) {
        color = "DARK " + color;
    } else if (l > 80.0) {
        color = "VERY LIGHT " + color;
    } else if (l > 60.0) {
        color = "LIGHT " + color;
    }

    // Adjust color based on saturation
    if (s < 20.0) {
        color = "VERY PALE " + color;
    } else if (s < 40.0) {
        color = "PALE " + color;
    } else if (s > 80.0) {
        color = "VERY VIVID " + color;
    } else if (s > 60.0) {
        color = "VIVID " + color;
    }
    return color;
  }
  int getAL() {
    return rawlux;
  }
  int getLux() {
    return lux;
  }
  int getIR() {
    return ir;
  }
private:
  LTR381RGBClass* _light = nullptr;
  int r, g, b, rawlux, lux, ir;
  int initialized = 0;
};

class _distance_api {
public:
  _distance_api(VL53L4CD* sensor) : sensor(sensor) {
    isVL53L4CD = true;
  };
  _distance_api(VL53L4ED* sensor) : sensor(sensor) {};
  uint8_t setRangeTiming(uint32_t timing_budget_ms, uint32_t inter_measurement_ms) {
    if (isVL53L4CD) {
      return ((VL53L4CD*)sensor)->VL53L4CD_SetRangeTiming(timing_budget_ms, inter_measurement_ms);
    } else {
      return ((VL53L4ED*)sensor)->VL53L4ED_SetRangeTiming(timing_budget_ms, inter_measurement_ms);
    }
  }
  uint8_t startRanging() {
    if (isVL53L4CD) {
      return ((VL53L4CD*)sensor)->VL53L4CD_StartRanging();
    } else {
      return ((VL53L4ED*)sensor)->VL53L4ED_StartRanging();
    }
  }
  uint8_t checkForDataReady(uint8_t* p_is_data_ready) {
    if (isVL53L4CD) {
      return ((VL53L4CD*)sensor)->VL53L4CD_CheckForDataReady(p_is_data_ready);
    } else {
      return ((VL53L4ED*)sensor)->VL53L4ED_CheckForDataReady(p_is_data_ready);
    }
  }
  uint8_t clearInterrupt() {
    if (isVL53L4CD) {
      return ((VL53L4CD*)sensor)->VL53L4CD_ClearInterrupt();
    } else {
      return ((VL53L4ED*)sensor)->VL53L4ED_ClearInterrupt();
    }
  }
  uint8_t getResult(void* result) {
    if (isVL53L4CD) {
      return ((VL53L4CD*)sensor)->VL53L4CD_GetResult((VL53L4CD_Result_t*)result);
    } else {
      return ((VL53L4ED*)sensor)->VL53L4ED_GetResult((VL53L4ED_ResultsData_t*)result);
    }
  }
private:
  void* sensor;
  bool isVL53L4CD = false;
};

class ModulinoDistance : public Module {
public:
  ModulinoDistance(ModulinoHubPort* hubPort = nullptr)
    : Module(0xFF, "DISTANCE", hubPort) {}
  bool begin() {

    if (hubPort != nullptr) {
      hubPort->select();
    }
    // try scanning for 0x29 since the library contains a while(true) on begin()
    getWire()->beginTransmission(0x29);
    if (getWire()->endTransmission() != 0) {
      if (hubPort != nullptr) {
        hubPort->clear();
      }
      return false;
    }
    tof_sensor = new VL53L4CD((TwoWire*)getWire(), -1);
    auto ret = tof_sensor->InitSensor();
    if (ret != VL53L4CD_ERROR_NONE) {
      delete tof_sensor;
      tof_sensor = nullptr;
      tof_sensor_alt = new VL53L4ED((TwoWire*)getWire(), -1);
      ret = tof_sensor_alt->InitSensor();
      if (ret == VL53L4ED_ERROR_NONE) {
        api = new _distance_api(tof_sensor_alt);
      } else {
        delete tof_sensor_alt;
        tof_sensor_alt = nullptr;
        if (hubPort != nullptr) {
          hubPort->clear();
        }
        return false;
      }
    } else {
      api = new _distance_api(tof_sensor);
    }

    __increaseI2CPriority();
    api->setRangeTiming(20, 0);
    api->startRanging();
    if (hubPort != nullptr) {
      hubPort->clear();
    }
    return true;
  }
  operator bool() {
    return (api != nullptr);
  }
  bool available() {
    if (api == nullptr) {
      return false;
    }

    if (hubPort != nullptr) {
      hubPort->select();
    }
    uint8_t NewDataReady = 0;
    api->checkForDataReady(&NewDataReady);
    if (NewDataReady) {
      api->clearInterrupt();
      api->getResult(&results);
    }
    if (hubPort != nullptr) {
      hubPort->clear();
    }
    if (results.range_status == 0) {
      internal = results.distance_mm;
    } else {
      internal = NAN;
    }
    return !isnan(internal);
  }
  float get() {
    return internal;
  }
private:
  VL53L4CD* tof_sensor = nullptr;
  VL53L4ED* tof_sensor_alt = nullptr;
  VL53L4CD_Result_t results;
  //VL53L4ED_ResultsData_t results;
  float internal = NAN;
  _distance_api* api = nullptr;
};

class ModulinoOptoRelay : public Module {
public:
  ModulinoOptoRelay(uint8_t address = 0xFF, ModulinoHubPort* hubPort = nullptr)
    : Module(address, "OPTO_RELAY", hubPort) {}
  ModulinoOptoRelay(ModulinoHubPort* hubPort, uint8_t address = 0xFF)
    : Module(address, "OPTO_RELAY", hubPort) {}
  bool update() {
    uint8_t buf[3];
    auto res = read((uint8_t*)buf, 3);
    auto ret = res && (buf[0] != last_status[0] || buf[1] != last_status[1] || buf[2] != last_status[2]);
    last_status[0] = buf[0];
    last_status[1] = buf[1];
    last_status[2] = buf[2];
    return ret;
  }
  void on() {
    uint8_t buf[3];
    buf[0] = 1;
    buf[1] = 0;
    buf[2] = 0;
    write((uint8_t*)buf, 3);
    return;
  }
  void off() {
    uint8_t buf[3];
    buf[0] = 0;
    buf[1] = 0;
    buf[2] = 0;
    write((uint8_t*)buf, 3);
    return;
  }
  bool getStatus() {
    update();
    return last_status[0];
  }
  virtual uint8_t discover() {
    for (unsigned int i = 0; i < sizeof(match)/sizeof(match[0]); i++) {
      if (scan(match[i])) {
        return match[i];
      }
    }
    return 0xFF;
  }
private:
  bool last_status[3];
protected:
  uint8_t match[1] = { 0x28 };  // same as fw main.c
};

class ModulinoLatchRelay : public Module {
public:
  ModulinoLatchRelay(uint8_t address = 0xFF, ModulinoHubPort* hubPort = nullptr)
    : Module(address, "LATCH_RELAY", hubPort) {}
  ModulinoLatchRelay(ModulinoHubPort* hubPort, uint8_t address = 0xFF)
    : Module(address, "LATCH_RELAY", hubPort) {}
  bool update() {
    uint8_t buf[3];
    auto res = read((uint8_t*)buf, 3);
    auto ret = res && (buf[0] != last_status[0] || buf[1] != last_status[1] || buf[2] != last_status[2]);
    last_status[0] = buf[0];
    last_status[1] = buf[1];
    last_status[2] = buf[2];
    return ret;
  }
  void set() {
    uint8_t buf[3];
    buf[0] = 1;
    buf[1] = 0;
    buf[2] = 0;
    write((uint8_t*)buf, 3);
    return;
  }
  void reset() {
    uint8_t buf[3];
    buf[0] = 0;
    buf[1] = 0;
    buf[2] = 0;
    write((uint8_t*)buf, 3);
    return;
  }
  int getStatus() {
    update();
    if (last_status[0] == 0 && last_status[1] == 0) {
      return -1; // unknown, last status before poweroff is maintained
    } else if (last_status[0] == 1) {
      return 0; // off
    } else {
      return 1; // on
    }
  }
  virtual uint8_t discover() {
    for (unsigned int i = 0; i < sizeof(match)/sizeof(match[0]); i++) {
      if (scan(match[i])) {
        return match[i];
      }
    }
    return 0xFF;
  }
private:
  bool last_status[3];
protected:
  uint8_t match[1] = { 0x04 };  // same as fw main.c
};

class ModulinoMicrophone : public Module {
public:
  ModulinoMicrophone(uint8_t address = 0xFF, ModulinoHubPort* hubPort = nullptr)
    : Module(address, "MICROPHONE", hubPort) {
      resetDecoder();
    }
  ModulinoMicrophone(ModulinoHubPort* hubPort, uint8_t address = 0xFF)
    : Module(address, "MICROPHONE", hubPort) {
      resetDecoder();
    }

  /**
   * @brief Finds the module and starts from a known state.
   * @note  The module remembers the mode a previous sketch left it in (it
   *        only forgets on power loss). Without resetting it here, an audio
   *        sketch run after a keyword spotting one would keep receiving
   *        detection packets instead of audio, and appear broken.
   */
  bool begin() {
    if (!Module::begin()) {
      return false;
    }
    setMode(MODE_STREAM);
    return true;
  }

  /**
   * @brief Resets the ADPCM decoder state (useful at startup or on sync loss)
   */
  void resetDecoder() {
    _predicted_sample = 0;
    _index = 0;
  }

  /**
   * @brief Fetches a new block of 64 linear PCM audio samples from the microphone module.
   * @return true if the I2C transmission succeeded.
   */
  bool update() {
    uint8_t adpcm_bytes[32];
  
    if (!read(adpcm_bytes, 32)) {
      return false;
    }

    resetDecoder();

    int sample_idx = 0;

    // Decode 32 ADPCM bytes into 64 linear PCM samples
    for (int i = 0; i < 32; i++) {
      uint8_t original_byte = adpcm_bytes[i];

      // Extract low and high 4-bit nibbles
      uint8_t low_nibble  = original_byte & 0x0F;
      uint8_t high_nibble = (original_byte >> 4) & 0x0F;

      _buffer[sample_idx++] = decodeSample(low_nibble);
      _buffer[sample_idx++] = decodeSample(high_nibble);
    }

    return true;
  }

  /**
   * @brief Gets a single signed 16-bit linear PCM audio sample from the current buffer.
   * @param index Sample index (0 to 63).
   * @return The 16-bit signed PCM sample value (-32768 to 32767).
   */
  int16_t getPcmSample(int index) {
    if (index >= 0 && index < 64) {
      return _buffer[index];
    }
    return 0;
  }

  /**
   * @brief Gets the pointer to the internal 16-bit signed linear PCM buffer.
   * @return Pointer to the int16_t array of 64 PCM samples.
   */
  int16_t* getPcmBuffer() {
    return _buffer;
  }

  /*
   * ==========================================================================
   *  Keyword spotting (on-board AI)
   * ==========================================================================
   * The module can run a small neural network locally and tell you when it
   * hears the keyword it was trained for, instead of streaming audio.
   * The active model lives in the module and survives power cycles: either
   * the one it was shipped with, or your own, uploaded once with
   * updateModel() (see the KWS_UpdateModel utility sketch).
   */

  /**
   * @brief Switches the module to keyword spotting: from now on the module
   *        listens locally and reports detections instead of streaming audio.
   * @return true if the module accepted the mode change.
   */
  bool beginKeywordSpotting() {
    _lastError = nullptr;
    if (!setMode(MODE_KEYWORD_SPOTTING)) {
      _lastError = "cannot switch the module to keyword spotting";
      return false;
    }
    return true;
  }

  /**
   * @brief Goes back to plain audio streaming (the default mode), so that
   *        update()/getPcmSample() work again.
   */
  bool endKeywordSpotting() {
    _lastError = nullptr;
    if (!setMode(MODE_STREAM)) {
      _lastError = "cannot switch the module back to audio streaming";
      return false;
    }
    return true;
  }

  /**
   * @brief Tells whether the keyword has been heard since the last call.
   *        Each spoken word is reported exactly once, so this can be polled
   *        as often as you like (10 times per second is plenty).
   * @return true once per detection.
   */
  bool keywordDetected() {
    KwsStatus status;

    if (!readStatus(status) || status.mode != MODE_KEYWORD_SPOTTING) {
      /* No status packet, or the module is not listening any more: it was
       * most likely reset, and it powers up in streaming mode. Put it back
       * into keyword spotting on its own, so the sketch keeps working; at
       * most once per second, so that an unplugged module does not slow
       * down the loop. */
      if (_kwsActive && (millis() - _lastRecovery) > 1000) {
        _lastRecovery = millis();
        uint8_t frame[2] = { CMD_SET_MODE, MODE_KEYWORD_SPOTTING };
        command(frame, sizeof(frame), 200);
      }
      return false;
    }

    if (!status.detected) {
      return false;
    }

    _confidence = status.confidence;
    _detections = status.eventCount;
    return true;
  }

  /**
   * @brief How sure the module was about the last detection, 0 to 100 %.
   */
  uint8_t keywordConfidence() {
    return (uint8_t)(((uint16_t)_confidence * 100u) / 255u);
  }

  /**
   * @brief Total number of detections since the module was powered on.
   */
  uint16_t keywordCount() {
    return _detections;
  }

  /**
   * @brief The word the module is currently listening for, e.g. "go".
   *        Read from the module itself, so it always matches the model in
   *        use - handy to tell the user what to say.
   */
  const char* keywordName() {
    if (_keyword[0] == '\0') {
      KwsStatus status;
      readStatus(status);  // fills the cache
    }
    return _keyword;
  }

  /*
   * --------------------------------------------------------------------------
   *  Model management
   * --------------------------------------------------------------------------
   */

  /** Optional callback to follow the upload progress (e.g. draw a bar). */
  typedef void (*ModelProgressCallback)(uint32_t bytesSent, uint32_t bytesTotal);

  /**
   * @brief Uploads a keyword spotting model into the module, replacing the
   *        one in use. The model is written to the spare memory slot and
   *        only activated once fully verified, so an interrupted upload
   *        leaves the previous model working.
   *        Takes a few seconds; the model then persists across power cycles.
   * @param package     Model package (.mmdl array produced by the training tool).
   * @param size        Size of the package in bytes.
   * @param onProgress  Optional progress callback.
   * @return true if the new model is active. On failure see lastError().
   */
  bool updateModel(const uint8_t* package, uint32_t size,
                   ModelProgressCallback onProgress = nullptr) {
    _lastError = nullptr;

    if (!isValidModel(package, size)) {
      _lastError = "not a valid model package";
      return false;
    }

    if (!setMode(MODE_MODEL_UPDATE)) {
      _lastError = "module did not enter model update mode";
      return false;
    }

    // Announce the transfer: size and checksum of the whole package, so the
    // module can prepare its memory and later verify what it received.
    uint32_t crc = crc32(package, size);
    uint8_t begin[9] = { CMD_MODEL_BEGIN,
                         (uint8_t)(size >> 24), (uint8_t)(size >> 16),
                         (uint8_t)(size >> 8),  (uint8_t)size,
                         (uint8_t)(crc >> 24),  (uint8_t)(crc >> 16),
                         (uint8_t)(crc >> 8),   (uint8_t)crc };
    // Erasing the destination slot takes a moment: allow a generous timeout.
    if (!command(begin, sizeof(begin), 5000)) {
      _lastError = "module refused the model transfer";
      setMode(MODE_STREAM);
      return false;
    }

    uint16_t seq = 0;
    for (uint32_t sent = 0; sent < size; seq++) {
      uint8_t chunk = (size - sent > MODEL_CHUNK) ? MODEL_CHUNK : (uint8_t)(size - sent);
      uint8_t frame[4 + MODEL_CHUNK];
      frame[0] = CMD_MODEL_DATA;
      frame[1] = (uint8_t)(seq >> 8);
      frame[2] = (uint8_t)seq;
      frame[3] = chunk;
      memcpy(&frame[4], &package[sent], chunk);

      if (!command(frame, 4 + chunk, 500)) {
        _lastError = "model transfer interrupted";
        setMode(MODE_STREAM);
        return false;
      }

      sent += chunk;
      if (onProgress != nullptr) {
        onProgress(sent, size);
      }
    }

    // Verify and activate: the module checks the checksum before switching.
    uint8_t commit[1] = { CMD_MODEL_COMMIT };
    if (!command(commit, sizeof(commit), 2000)) {
      _lastError = "model rejected by the module (corrupted or incompatible)";
      setMode(MODE_STREAM);
      return false;
    }

    setMode(MODE_STREAM);
    return true;
  }

  /**
   * @brief Goes back to the model the module was shipped with. Instant: the
   *        factory model is always kept in the module and never overwritten.
   */
  bool restoreFactoryModel() {
    _lastError = nullptr;

    // Leave streaming first: while streaming, the module answers with audio
    // and there would be no way to read the outcome of the command.
    if (!setMode(MODE_MODEL_UPDATE)) {
      _lastError = "module not responding";
      return false;
    }

    uint8_t restore[1] = { CMD_MODEL_RESTORE };
    bool ok = command(restore, sizeof(restore), 2000);
    if (!ok) {
      _lastError = "the module could not restore the factory model";
    }

    setMode(MODE_STREAM);
    return ok;
  }

  /**
   * @brief Why the last operation failed, as readable text (nullptr if none).
   */
  const char* lastError() {
    return _lastError;
  }

  /**
   * @brief The keyword a model package was trained for, e.g. "go".
   *        Reads it from the package itself, no module needed.
   */
  static const char* modelKeyword(const uint8_t* package) {
    static char keyword[MODEL_NAME_LEN + 1];
    memcpy(keyword, &package[MODEL_NAME_OFFSET], MODEL_NAME_LEN);
    keyword[MODEL_NAME_LEN] = '\0';
    return keyword;
  }

  /**
   * @brief Sanity check of a model package before sending it to the module.
   */
  static bool isValidModel(const uint8_t* package, uint32_t size) {
    if (package == nullptr || size <= MODEL_HEADER_LEN) {
      return false;
    }
    // Packages start with the "MMDL" marker...
    if (memcmp(package, "MMDL", 4) != 0) {
      return false;
    }
    // ...and declare a payload length that must match the array size.
    uint32_t payload = (uint32_t)package[8] | ((uint32_t)package[9] << 8) |
                       ((uint32_t)package[10] << 16) | ((uint32_t)package[11] << 24);
    return (payload + MODEL_HEADER_LEN) == size;
  }

  virtual uint8_t discover() {
    for (unsigned int i = 0; i < sizeof(match)/sizeof(match[0]); i++) {
      if (scan(match[i])) {
        return match[i];
      }
    }
    return 0xFF;
  }

private:
  int16_t _buffer[64]; // Output buffer for decoded PCM samples

  // ADPCM decoder internal state
  int16_t _predicted_sample;
  int8_t _index;

  /* ---------------- Keyword spotting: protocol and internals -------------
   * Wire format of the module (mirrors kws_defs.h in the node firmware).
   * Kept private: sketches use the methods above, never these values. */
  enum : uint8_t {
    MODE_STREAM           = 0,     // audio streaming (module default)
    MODE_KEYWORD_SPOTTING = 1,
    MODE_MODEL_UPDATE     = 2,

    CMD_SET_MODE          = 0xA0,
    CMD_MODEL_BEGIN       = 0xB0,
    CMD_MODEL_DATA        = 0xB1,
    CMD_MODEL_COMMIT      = 0xB2,
    CMD_MODEL_RESTORE     = 0xC0,

    FRAME_SIZE            = 40,    // every command frame is padded to this
    MODEL_CHUNK           = 32,    // model bytes per frame
    STATUS_PACKET         = 24,    // reply length in the non-streaming modes
    STATUS_KEYWORD_OFFSET = 8,     // where the active keyword starts in it
    PACKET_MAGIC          = 0x4B,  // marks a reply as a status packet
    STATUS_OK             = 0x5A,

    MODEL_HEADER_LEN      = 40,    // .mmdl header
    MODEL_NAME_OFFSET     = 16,
    MODEL_NAME_LEN        = 16,
  };

  struct KwsStatus {
    uint8_t  mode;
    uint8_t  lastStatus;
    bool     detected;
    uint8_t  confidence;
    uint16_t eventCount;
    uint8_t  abiVersion;
  };

  const char* _lastError = nullptr;
  uint8_t     _confidence = 0;
  uint16_t    _detections = 0;
  bool        _kwsActive = false;
  uint32_t    _lastRecovery = 0;
  char        _keyword[MODEL_NAME_LEN + 1] = "";

  /** Sends a command, zero padded to the fixed frame length. */
  bool sendFrame(const uint8_t* payload, uint8_t len) {
    uint8_t frame[FRAME_SIZE] = { 0 };
    memcpy(frame, payload, len > FRAME_SIZE ? FRAME_SIZE : len);
    return write(frame, FRAME_SIZE);
  }

  /**
   * Reads the status packet. Fails when the module is not in a mode that
   * produces one (in streaming mode the reply is audio, not status), which
   * is also how we notice that the module has been reset behind our back.
   */
  bool readStatus(KwsStatus& out) {
    if (getAddress() >= 0x7F) {
      return false;
    }
    if (hubPort != nullptr) {
      hubPort->select();
    }

    bool ok = false;
    if (getWire()->requestFrom(getAddress(), (uint8_t)STATUS_PACKET) >= STATUS_PACKET) {
      uint8_t p[STATUS_PACKET];
      for (uint8_t i = 0; i < STATUS_PACKET; i++) {
        p[i] = getWire()->read();
      }
      if (p[0] == PACKET_MAGIC) {
        out.mode       = p[1];
        out.lastStatus = p[2];
        out.detected   = (p[3] != 0);
        out.confidence = p[4];
        out.eventCount = (uint16_t)p[5] | ((uint16_t)p[6] << 8);
        out.abiVersion = p[7];
        // Cache the active keyword: every status read refreshes it, so
        // keywordName() never has to spend an extra read (which would
        // swallow a pending detection).
        memcpy(_keyword, &p[STATUS_KEYWORD_OFFSET], MODEL_NAME_LEN);
        _keyword[MODEL_NAME_LEN] = '\0';
        ok = true;
      }
    }
    while (getWire()->available()) {
      getWire()->read();
    }

    if (hubPort != nullptr) {
      hubPort->clear();
    }
    return ok;
  }

  /**
   * Sends a command and waits for the module to report the outcome.
   * While the module is busy writing its flash memory it simply does not
   * answer, so "no reply yet" means "still working": keep asking until it
   * does, or until the timeout expires.
   */
  bool command(const uint8_t* payload, uint8_t len, uint16_t timeout_ms) {
    if (!sendFrame(payload, len)) {
      return false;  // module never discovered on the bus
    }

    uint32_t start = millis();
    do {
      delay(2);  // give the module the time to process the frame
      KwsStatus status;
      if (readStatus(status)) {
        return (status.lastStatus == STATUS_OK);
      }
    } while ((millis() - start) < timeout_ms);

    return false;
  }

  bool setMode(uint8_t mode) {
    uint8_t frame[2] = { CMD_SET_MODE, mode };

    if (mode == MODE_STREAM) {
      // From now on the module replies with audio, not with status packets,
      // so there is nothing to read back as confirmation.
      bool sent = sendFrame(frame, sizeof(frame));
      delay(5);
      resetDecoder();  // the audio decoder must restart from a known state
      _kwsActive = false;
      return sent;
    }

    bool ok = command(frame, sizeof(frame), 500);
    _kwsActive = ok && (mode == MODE_KEYWORD_SPOTTING);
    return ok;
  }

  /** CRC32 (IEEE 802.3), same algorithm the module uses to verify uploads. */
  static uint32_t crc32(const uint8_t* data, uint32_t len) {
    static const uint32_t table[16] = {
      0x00000000, 0x1DB71064, 0x3B6E20C8, 0x26D930AC,
      0x76DC4190, 0x6B6B51F4, 0x4DB26158, 0x5005713C,
      0xEDB88320, 0xF00F9344, 0xD6D6A3E8, 0xCB61B38C,
      0x9B64C2B0, 0x86D3D2D4, 0xA00AE278, 0xBDBDF21C
    };
    uint32_t crc = 0xFFFFFFFF;
    while (len--) {
      crc ^= *data++;
      crc = (crc >> 4) ^ table[crc & 0x0F];
      crc = (crc >> 4) ^ table[crc & 0x0F];
    }
    return ~crc;
  }

  // Standard IMA ADPCM tables
  const int IndexTable[16] = {
      -1, -1, -1, -1, 2, 4, 6, 8,
      -1, -1, -1, -1, 2, 4, 6, 8
  };

  const int StepTable[89] = {
      7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
      19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
      50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
      130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
      337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
      876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
      2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
      5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
      15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
  };

  /**
   * @brief Decodes a single 4-bit ADPCM nibble into a 16-bit linear PCM sample.
   */
  int16_t decodeSample(uint8_t code) {
    int step = StepTable[_index];
    
    // Inverse quantization
    int diffq = step >> 3;
    if (code & 4) diffq += step;
    if (code & 2) diffq += (step >> 1);
    if (code & 1) diffq += (step >> 2);
    
    // Apply sign bit (Bit 3)
    long pred = _predicted_sample;
    if (code & 8) {
      pred -= diffq;
    } else {
      pred += diffq;
    }
    
    // Clamp to 16-bit signed integer limits
    if (pred > 32767) pred = 32767;
    else if (pred < -32768) pred = -32768;
    
    // Update step index
    _index += IndexTable[code & 7];
    if (_index < 0) _index = 0;
    if (_index > 88) _index = 88;
    
    _predicted_sample = (int16_t)pred;
    return _predicted_sample;
  }

protected:
  uint8_t match[1] = { 0x54 }; 
};

#endif
