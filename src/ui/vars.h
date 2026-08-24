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
extern const char *get_var_pomodoro_stage();
extern void set_var_pomodoro_stage(const char *value);
extern const char *get_var_pomodoro_timer();
extern void set_var_pomodoro_timer(const char *value);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_VARS_H*/