SET(SOURCES ui_comp_wifiicon.c
    ui_comp.c
    ui_Screen1.c
    ui.c
    ui_comp_hook.c
    ui_helpers.c
    ui_img_wifiicon_png.c
    ui_img_chargingicon_png.c)

add_library(ui ${SOURCES})
