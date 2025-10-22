#pragma once
#include <TFT_eSPI.h>
#include <lvgl.h>

// Minimal HAL that sets up LVGL draw buffers, display driver, and a dummy input
// device. Owns the TFT_eSPI instance and runs lv_timer_handler() in loop().
class Display {
   public:
    // rotation: 0..3 like TFT_eSPI
    bool begin(uint16_t w, uint16_t h, uint8_t rotation = 0);

    // call every loop iteration
    inline void loop() { lv_timer_handler(); }

    // Accessors
    inline lv_disp_t* lvDisplay() const { return _lv_disp; }
    inline uint16_t width() const { return _w; }
    inline uint16_t height() const { return _h; }

   private:
    // LVGL callbacks (static thunks -> instance)
    static void _flush_cb(lv_disp_drv_t* disp, const lv_area_t* area,
                          lv_color_t* color_p);
    static void _touch_read_cb(lv_indev_drv_t* indev, lv_indev_data_t* data);

    // Instance methods
    void _flush(const lv_area_t* area, lv_color_t* color_p);

    // Members
    uint16_t _w = 0, _h = 0;
    TFT_eSPI _tft{_w, _h};  // TFT_eSPI can accept (w,h) ctor
    lv_disp_t* _lv_disp = nullptr;

    lv_color_t* _buf1 = nullptr;
    lv_disp_draw_buf_t _draw_buf{};
    lv_disp_drv_t _disp_drv{};
    lv_indev_drv_t _indev_drv{};
};
