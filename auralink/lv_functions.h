#pragma once
#include "lvgl.h"

static inline bool lv_obj_ok(const lv_obj_t *o) { return o && lv_obj_is_valid(o); }

#define LV_SAFE(o, stmt)        do { if (lv_obj_ok((const lv_obj_t*)(o))) { stmt; } } while (0)
#define LV_SAFE_DO(o, code)     do { lv_obj_t* _o = (lv_obj_t*)(o); if (lv_obj_ok(_o)) { code; } } while (0)

#define LV_CHILD(parent, comp_id) ((lv_obj_t*)ui_comp_get_child((lv_obj_t*)(parent), (comp_id)))

static inline lv_obj_t* LV_CHILD_SAFE(lv_obj_t *parent, uint32_t comp_id) {
    if (!lv_obj_ok(parent)) return NULL;
    lv_obj_t *c = LV_CHILD(parent, comp_id);
    return lv_obj_ok(c) ? c : NULL;
}

static void _LV_invalidate_on_delete(lv_event_t *e) {
    lv_obj_t **slot = (lv_obj_t**)lv_event_get_user_data(e);
    if (slot) *slot = NULL;
}

static inline void LV_WATCH_DELETE(lv_obj_t *o, lv_obj_t **slot) {
    if (lv_obj_ok(o)) lv_obj_add_event_cb(o, _LV_invalidate_on_delete, LV_EVENT_DELETE, slot);
}

#define LV_TRY_FIND(dst, parent, comp_id) do {                               \
    if (!lv_obj_ok((dst))) {                                                 \
        (dst) = LV_CHILD_SAFE((lv_obj_t*)(parent), (comp_id));               \
        if (lv_obj_ok((dst))) LV_WATCH_DELETE((dst), &(dst));                \
    }                                                                        \
} while (0)

#define LV_ICON_RECOLOR_SAFE(o, col)                                         \
    LV_SAFE((o), do {                                                        \
        lv_obj_set_style_img_recolor((o), (col), LV_PART_MAIN);              \
        lv_obj_set_style_img_recolor_opa((o), LV_OPA_COVER, LV_PART_MAIN);   \
    } while (0))