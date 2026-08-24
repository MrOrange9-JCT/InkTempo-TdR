#include "vars.h"

static const char *battery_level = "100%";
static const char *screen_title = "EPDiy T5-S3";
static const char *current_time = "12:00";
static const char *pomodoro_stage = "Ready";
static const char *pomodoro_timer = "25:00";

const char *get_var_battery_level() {
    return battery_level;
}

void set_var_battery_level(const char *value) {
    battery_level = value;
}

const char *get_var_screen_title() {
    return screen_title;
}

void set_var_screen_title(const char *value) {
    screen_title = value;
}

const char *get_var_current_time() {
    return current_time;
}

void set_var_current_time(const char *value) {
    current_time = value;
}

const char *get_var_pomodoro_stage() {
    return pomodoro_stage;
}

void set_var_pomodoro_stage(const char *value) {
    pomodoro_stage = value;
}

const char *get_var_pomodoro_timer() {
    return pomodoro_timer;
}

void set_var_pomodoro_timer(const char *value) {
    pomodoro_timer = value;
}
