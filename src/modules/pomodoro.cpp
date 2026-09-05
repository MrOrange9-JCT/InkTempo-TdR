#include "pomodoro.h"
#include "stats.h"

#include <Arduino.h>

// Temporitzador Pomodoro
static const uint32_t FOCUS_DURATION_MS = 25 * 60 * 1000; // 25 min
static const uint32_t SHORT_BREAK_DURATION_MS = 5 * 60 * 1000; // 5 min
static const uint32_t LONG_BREAK_DURATION_MS = 10 * 60 * 1000; // 10 min
static const uint8_t CYCLES_BEFORE_LONG_BREAK = 2;

static PomodoroStage current_stage = POMODORO_READY;
static PomodoroStage paused_from_stage = POMODORO_READY;
static unsigned long start_time = 0;
static unsigned long elapsed_time = 0;
static uint8_t cycles_completed = 0;
static String label = "Llest";

static unsigned long get_stage_duration(PomodoroStage stage) {
	switch (stage) {
		case POMODORO_FOCUS:
			return FOCUS_DURATION_MS;
		case POMODORO_SHORT_BREAK:
			return SHORT_BREAK_DURATION_MS;
		case POMODORO_LONG_BREAK:
			return LONG_BREAK_DURATION_MS;
		default:
			return 0;
	}
}

static void start_stage(PomodoroStage stage) {
	current_stage = stage;
	start_time = millis();
	elapsed_time = 0;

	switch (stage) {
		case POMODORO_FOCUS:
			label = "Concentració #" + String(cycles_completed + 1); // TODO: Confirmar si es mostra el número correcte
			break;
		case POMODORO_SHORT_BREAK:
			label = "Descans Curt #" + String(cycles_completed + 1); // TODO: Confirmar si es mostra el número correcte
			break;
		case POMODORO_LONG_BREAK:
			label = "Descans Llarg #" + String(cycles_completed + 1); // TODO: Confirmar si es mostra el número correcte
			break;
		default:
			label = "Llest";
			break;
	}
}

void pomodoro_pause() {
	if (current_stage == POMODORO_READY || current_stage == POMODORO_PAUSED) return;

	paused_from_stage = current_stage;
	current_stage = POMODORO_PAUSED;
	elapsed_time += millis() - start_time;
}

extern "C" void pomodoro_toggle() {
	// Alternar entre pausar i reprendre el temporitzador Pomodoro, connectat amb el botó de la interfície d'usuari
	switch (current_stage) {
		case POMODORO_READY:
			cycles_completed = 0;
			start_stage(POMODORO_FOCUS);
			break;
		case POMODORO_FOCUS:
		case POMODORO_SHORT_BREAK:
		case POMODORO_LONG_BREAK:
			pomodoro_pause();
			break;
		case POMODORO_PAUSED:
			current_stage = paused_from_stage;
			start_time = millis();
			break;
	}
}

void pomodoro_update() {
	if (current_stage == POMODORO_PAUSED || current_stage == POMODORO_READY) return;

	const unsigned long elapsed = elapsed_time + (millis() - start_time);
	if (elapsed < get_stage_duration(current_stage)) return;

	if (current_stage == POMODORO_FOCUS) {
		cycles_completed++;
		// Si el residu és 0 --> descans llarg
		start_stage(cycles_completed % CYCLES_BEFORE_LONG_BREAK == 0
						? POMODORO_LONG_BREAK
						: POMODORO_SHORT_BREAK);
	} else {
		start_stage(POMODORO_FOCUS);
	}
}

PomodoroStage pomodoro_get_stage() {
	return current_stage;
}

const char *pomodoro_get_label() {
	if (current_stage == POMODORO_PAUSED) return "Pausat";
	return label.c_str();
}

unsigned long pomodoro_get_remaining_minutes() {
	if (current_stage == POMODORO_READY) return 0;

	// Si està pausat, utilitzar l'etapa des de la qual es va pausar per calcular el temps restant
	const PomodoroStage effective_stage = current_stage == POMODORO_PAUSED
											  ? paused_from_stage
											  : current_stage;
	const unsigned long duration = get_stage_duration(effective_stage);
	unsigned long elapsed = elapsed_time;
	if (current_stage != POMODORO_PAUSED) elapsed += millis() - start_time;
	if (elapsed >= duration) return 0;

	// Arrodonir (truncar) al minut
	return ((duration - elapsed) / 1000 + 59) / 60;

	// Si està en sessio de concentració, sumar els minuts passats a les estadístiques
	if (current_stage == POMODORO_FOCUS) {
		stats_add_focus_minutes(((duration - elapsed) / 1000 + 59) / 60);
	}
}
