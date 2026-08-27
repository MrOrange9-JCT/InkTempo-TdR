#include "vars.h"
#include <string.h>

#define VAR_BUF_SIZE 32

static char battery_level[VAR_BUF_SIZE] = "100%";
static char screen_title[VAR_BUF_SIZE] = "EPDiy T5-S3";
static char current_time[VAR_BUF_SIZE] = "12:00";
static char pomodoro_stage_label[VAR_BUF_SIZE] = "Preparat";
static char pomodoro_timer[VAR_BUF_SIZE] = "25′";

static void set_var(char *dest, size_t dest_size, const char *value) {
    strncpy(dest, value, dest_size - 1);
    dest[dest_size - 1] = '\0';
}

const char *get_var_battery_level() {
    return battery_level;
}

void set_var_battery_level(const char *value) {
    set_var(battery_level, sizeof(battery_level), value);
}

const char *get_var_screen_title() {
    return screen_title;
}

void set_var_screen_title(const char *value) {
    set_var(screen_title, sizeof(screen_title), value);
}

const char *get_var_current_time() {
    return current_time;
}

void set_var_current_time(const char *value) {
    set_var(current_time, sizeof(current_time), value);
}

const char *get_var_pomodoro_stage() {
    return pomodoro_stage_label;
}

void set_var_pomodoro_stage_label(const char *value) {
    set_var(pomodoro_stage_label, sizeof(pomodoro_stage_label), value);
}

const char *get_var_pomodoro_timer() {
    return pomodoro_timer;
}

void set_var_pomodoro_timer(const char *value) {
    set_var(pomodoro_timer, sizeof(pomodoro_timer), value);
}