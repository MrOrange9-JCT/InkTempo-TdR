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
    lv_obj_t *content;
    lv_obj_t *obj0;
    lv_obj_t *obj0__obj12;
    lv_obj_t *obj0__obj13;
    lv_obj_t *obj0__obj14;
    lv_obj_t *obj0__obj15;
    lv_obj_t *obj0__obj16;
    lv_obj_t *obj0__obj17;
    lv_obj_t *obj0__obj18;
    lv_obj_t *obj0__obj19;
    lv_obj_t *obj0__obj20;
    lv_obj_t *obj0__obj21;
    lv_obj_t *obj0__obj22;
    lv_obj_t *obj0__obj23;
    lv_obj_t *obj0__obj24;
    lv_obj_t *obj1;
    lv_obj_t *obj1__task_2;
    lv_obj_t *obj1__obj0;
    lv_obj_t *obj1__obj1;
    lv_obj_t *obj1__obj2;
    lv_obj_t *obj1__obj3;
    lv_obj_t *obj1__task_1;
    lv_obj_t *obj1__obj4;
    lv_obj_t *obj1__obj5;
    lv_obj_t *obj1__obj6;
    lv_obj_t *obj1__obj7;
    lv_obj_t *obj1__task_0;
    lv_obj_t *obj1__obj8;
    lv_obj_t *obj1__obj9;
    lv_obj_t *obj1__obj10;
    lv_obj_t *obj1__obj11;
    lv_obj_t *obj2;
    lv_obj_t *obj2__pomodoro_timer;
    lv_obj_t *obj2__pomodoro_stage;
    lv_obj_t *obj2__toggle_pomodoro;
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