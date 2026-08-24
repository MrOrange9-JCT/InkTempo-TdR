#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Screens

enum ScreensEnum {
    _SCREEN_ID_FIRST = 1,
    SCREEN_ID_MAIN = 1,
    _SCREEN_ID_LAST = 1
};

typedef struct _objects_t {
    lv_obj_t *main;
    lv_obj_t *title;
    lv_obj_t *top_line;
    lv_obj_t *bottom_line;
    lv_obj_t *current_time;
    lv_obj_t *battery;
    lv_obj_t *bottom_dots;
    lv_obj_t *dot_0;
    lv_obj_t *dot_1;
    lv_obj_t *dot_2;
} objects_t;

extern objects_t objects;

void create_screen_main();
void tick_screen_main();

void create_user_widget_pomodoro_screen(lv_obj_t *parent_obj, int startWidgetIndex);
void tick_user_widget_pomodoro_screen(int startWidgetIndex);

void create_user_widget_tasks_screen(lv_obj_t *parent_obj, int startWidgetIndex);
void tick_user_widget_tasks_screen(int startWidgetIndex);

void create_user_widget_stats_screen(lv_obj_t *parent_obj, int startWidgetIndex);
void tick_user_widget_stats_screen(int startWidgetIndex);

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/