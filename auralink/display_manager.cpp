#include "display_manager.h"
#include <Arduino.h>

DisplayManager* DisplayManager::_self = nullptr;

// Delegating ctor keeps default Params but avoids header default-arg issue
DisplayManager::DisplayManager(int leftPin, int rightPin)
: DisplayManager(leftPin, rightPin, Params{}) {}

DisplayManager::DisplayManager(int leftPin, int rightPin, const Params& p)
: _left{leftPin}, _right{rightPin}, _p(p) {}

static uint8_t _first_valid_idx(lv_obj_t** arr, uint8_t n) {
  for (uint8_t i = 0; i < n; ++i) if (arr[i]) return i;
  return 0xFF; // none
}

void DisplayManager::begin(lv_obj_t** screens, uint8_t screen_count) {
  _screens = screens;
  _count   = screen_count;
  _idx     = 0;

  // --- Sanity: count how many non-null screens we actually have
  uint8_t valid = 0;
  for (uint8_t i = 0; i < _count; ++i) if (_screens[i]) ++valid;

  if (valid == 0) {
    Serial.println("[DisplayManager] ERROR: All screens are null. Did ui_init() run? Do names match?");
    // Keep running but do not create timer; avoids crash
    _screens = nullptr; _count = 0;
    return;
  }

  uint8_t first = _first_valid_idx(_screens, _count);
  if (first == 0xFF) {
    Serial.println("[DisplayManager] ERROR: No valid screen index found.");
    _screens = nullptr; _count = 0;
    return;
  }

  _idx = first;
  Serial.printf("[DisplayManager] begin: %u screen slots, %u valid. First=%u\n", _count, valid, _idx);

  // Load the first non-null screen
  lv_scr_load(_screens[_idx]);

  if (_p.set_pinmode) {
    pinMode(_left.pin,  _p.pinmode ? _p.pinmode : INPUT);
    pinMode(_right.pin, _p.pinmode ? _p.pinmode : INPUT);
  }

  _self  = this;
  _timer = lv_timer_create(&_rotate_cb, _p.rotate_interval_ms, nullptr);
}

void DisplayManager::loop() { _readButtons(); }

void DisplayManager::next() {
  if (!_count || !_screens) return;
  // find next non-null
  for (uint8_t step = 1; step <= _count; ++step) {
    uint8_t n = (uint8_t)((_idx + step) % _count);
    if (_screens[n]) { _load(n, LV_SCR_LOAD_ANIM_MOVE_LEFT); _resetTimer(); return; }
  }
  // no valid target found; do nothing
}

void DisplayManager::prev() {
  if (!_count || !_screens) return;
  for (uint8_t step = 1; step <= _count; ++step) {
    uint8_t p = (uint8_t)((_idx + _count - step) % _count);
    if (_screens[p]) { _load(p, LV_SCR_LOAD_ANIM_MOVE_RIGHT); _resetTimer(); return; }
  }
}

void DisplayManager::setActive(uint8_t idx) {
  if (!_count || !_screens) return;
  idx %= _count;
  if (!_screens[idx]) {
    Serial.printf("[DisplayManager] WARN: setActive(%u) is null; ignored.\n", idx);
    return;
  }
  _load(idx, LV_SCR_LOAD_ANIM_FADE_ON);
  _resetTimer();
}

/* ---------- private ---------- */
void DisplayManager::_rotate_cb(lv_timer_t* /*t*/) {
  if (_self) _self->_rotate();
}

void DisplayManager::_rotate() {
  if (!_count || !_screens) return;
  // rotate to next non-null
  for (uint8_t step = 1; step <= _count; ++step) {
    uint8_t n = (uint8_t)((_idx + step) % _count);
    if (_screens[n]) { _load(n, LV_SCR_LOAD_ANIM_MOVE_LEFT); return; }
  }
  // no move if only current is valid
}

void DisplayManager::_resetTimer() {
  if (_timer) lv_timer_reset(_timer);
}

void DisplayManager::_load(uint8_t idx, lv_scr_load_anim_t anim) {
  if (!_screens || !_screens[idx]) {
    Serial.printf("[DisplayManager] ERROR: _load(%u) null screen; skipped.\n", idx);
    return;
  }
  lv_scr_load_anim(_screens[idx], anim, _p.anim_time_ms, 0, false);
  _idx = idx;
}

// Returns true on press *edge*
bool DisplayManager::_pollBtn(Btn& b) {
  uint32_t now = millis();
  int raw = digitalRead(b.pin);
  bool pressed = _p.active_high ? (raw == HIGH) : (raw == LOW);

  if (pressed != b.lastPressed) {
    if (now - b.lastChangeMs >= _p.debounce_ms) {
      b.lastPressed = pressed;
      b.lastChangeMs = now;
      if (pressed) return true; // rising edge
    }
  }
  return false;
}

void DisplayManager::_readButtons() {
  if (_pollBtn(_left))  prev();
  if (_pollBtn(_right)) next();
}
