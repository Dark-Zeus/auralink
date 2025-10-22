#pragma once
#include <lvgl.h>

class DisplayManager {
public:
  struct Params {
    uint32_t rotate_interval_ms = 5000;
    uint32_t anim_time_ms       = 300;
    uint32_t debounce_ms        = 90;
    bool     active_high        = true;
    bool     set_pinmode        = true;
    uint8_t  pinmode            = 0x0;
  };

  DisplayManager(int leftPin, int rightPin);
  DisplayManager(int leftPin, int rightPin, const Params& p);
  ~DisplayManager() = default;

  void begin(lv_obj_t** screens, uint8_t screen_count);

  void loop();
  void next();
  void prev();
  void setActive(uint8_t idx);

private:
  struct Btn {
    int pin;
    bool lastPressed = false;
    uint32_t lastChangeMs = 0;
  };

  static void _rotate_cb(lv_timer_t* t);
  void _rotate();
  void _resetTimer();
  void _load(uint8_t idx, lv_scr_load_anim_t anim);
  bool _pollBtn(Btn& b);
  void _readButtons();

  lv_obj_t** _screens = nullptr;
  uint8_t    _count   = 0;
  uint8_t    _idx     = 0;

  lv_timer_t* _timer  = nullptr;

  Btn     _left, _right;
  Params  _p;

  static DisplayManager* _self;
};
