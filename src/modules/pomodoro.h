#pragma once

#include <stdint.h>

enum PomodoroStage {
	POMODORO_READY,
	POMODORO_FOCUS,
	POMODORO_SHORT_BREAK,
	POMODORO_LONG_BREAK,
	POMODORO_PAUSED
};

extern "C" void pomodoro_toggle();

void pomodoro_update();
void pomodoro_pause();
PomodoroStage pomodoro_get_stage();
const char *pomodoro_get_label();
unsigned long pomodoro_get_remaining_minutes();
