SET(SOURCES ui_comp_notificationbar.c
    ui_comp.c
    ui_SensorData.c
    ui_DailyQuote.c
    ui_EmailSummary.c
    ui.c
    ui_comp_hook.c
    ui_helpers.c
    ui_img_pub_png.c
    ui_img_sub_png.c
    ui_img_wifiicon_png.c
    ui_img_chargingicon_png.c
    ui_img_aq_png.c
    ui_img_temperature_png.c
    ui_img_pressure_png.c
    ui_img_rh_png.c
    ui_img_light_png.c
    ui_img_uv_png.c)

add_library(ui ${SOURCES})
