#include "display.h"

#include <Arduino.h>

bool Display::begin(uint16_t w, uint16_t h, uint8_t rotation) {
  _w = w; _h = h;
  _tft = TFT_eSPI(_w, _h);   // re-construct with dimensions

  lv_init();

  _tft.begin();
  _tft.setRotation(rotation);

  // allocate a modest LVGL draw buffer (1/10th of screen)
  size_t buf_px = (_w * _h) / 10;
  _buf1 = (lv_color_t*) heap_caps_malloc(buf_px * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  if (!_buf1) {
    // fallback to regular heap
    _buf1 = (lv_color_t*) malloc(buf_px * sizeof(lv_color_t));
  }
  if (!_buf1) return false;

  lv_disp_draw_buf_init(&_draw_buf, _buf1, nullptr, buf_px);

  // display driver
  lv_disp_drv_init(&_disp_drv);
  _disp_drv.hor_res = _w;
  _disp_drv.ver_res = _h;
  _disp_drv.draw_buf = &_draw_buf;
  _disp_drv.flush_cb = &Display::_flush_cb;
  _disp_drv.user_data = this;            // pass instance to callback
  _lv_disp = lv_disp_drv_register(&_disp_drv);

  // dummy pointer input device (we're using separate capacitive keys)
  lv_indev_drv_init(&_indev_drv);
  _indev_drv.type = LV_INDEV_TYPE_POINTER;
  _indev_drv.read_cb = &Display::_touch_read_cb;
  _indev_drv.user_data = this;
  lv_indev_drv_register(&_indev_drv);

  return true;
}

/* static */ void Display::_flush_cb(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p) {
  auto* self = static_cast<Display*>(disp->user_data);
  self->_flush(area, color_p);
}

void Display::_flush(const lv_area_t* area, lv_color_t* color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  _tft.startWrite();
  _tft.setAddrWindow(area->x1, area->y1, w, h);
  _tft.pushColors((uint16_t*)&color_p->full, w * h, true);
  _tft.endWrite();

  lv_disp_flush_ready(&_disp_drv);
}

/* static */ void Display::_touch_read_cb(lv_indev_drv_t* indev, lv_indev_data_t* data) {
  // No TFT touch; keep LVGL happy with "not pressed"
  data->state = LV_INDEV_STATE_REL;
  data->point.x = 0; data->point.y = 0;
}
