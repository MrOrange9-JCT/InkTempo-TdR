#ifndef EEZ_LVGL_UI_VARS_H
#define EEZ_LVGL_UI_VARS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// enum declarations

// Flow global variables

enum FlowGlobalVariables {
    FLOW_GLOBAL_VARIABLE_POMODORO_STAGE = 0,
    FLOW_GLOBAL_VARIABLE_POMODORO_TIMER = 1
};

// Native global variables

extern const char *get_var_battery_level();
extern void set_var_battery_level(const char *value);
extern const char *get_var_screen_title();
extern void set_var_screen_title(const char *value);
extern const char *get_var_current_time();
extern void set_var_current_time(const char *value);
extern const char *get_var_pomodoro_stage_label();
extern void set_var_pomodoro_stage_label(const char *value);
extern const char *get_var_pomodoro_timer();
extern void set_var_pomodoro_timer(const char *value);
extern int32_t get_var_focus_counter_value();
extern void set_var_focus_counter_value(int32_t value);
extern int32_t get_var_focus_counter_goal();
extern void set_var_focus_counter_goal(int32_t value);
extern int32_t get_var_task_counter_value();
extern void set_var_task_counter_value(int32_t value);
extern int32_t get_var_task_counter_goal();
extern void set_var_task_counter_goal(int32_t value);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_VARS_H*/