#include "pomodoro.h"
#include "stats.h"
#include "config.h"

#include <Arduino.h>

// Temporitzador Pomodoro
static const uint32_t FOCUS_DURATION_MS = POMODORO_FOCUS_MINUTES * 60 * 1000;
static const uint32_t SHORT_BREAK_DURATION_MS = POMODORO_SHORT_BREAK_MINUTES * 60 * 1000; // 5 min
static const uint32_t LONG_BREAK_DURATION_MS = POMODORO_LONG_BREAK_MINUTES * 60 * 1000; // 10 min
static const uint8_t CYCLES_BEFORE_LONG_BREAK = POMODORO_CYCLES_BEFORE_LONG_BREAK;

static PomodoroStage current_stage = POMODORO_READY;
static PomodoroStage paused_from_stage = POMODORO_READY;
static unsigned long start_time = 0;
static unsigned long elapsed_time = 0;
static uint8_t focus_count = 0;
static uint8_t short_break_count = 0;
static uint8_t long_break_count = 0;
static String label = "Llest";

static void set_stage_label(PomodoroStage stage) {
    switch (stage) {
        case POMODORO_FOCUS:
            label = "Concentració #" + String(focus_count);
            break;
        case POMODORO_SHORT_BREAK:
            label = "Descans Curt #" + String(short_break_count);
            break;
        case POMODORO_LONG_BREAK:
            label = "Descans Llarg #" + String(long_break_count);
            break;
        default:
            label = "Llest";
            break;
    }
}

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
    set_stage_label(stage);
}

static void transition_to_stage(PomodoroStage next_stage) {
    if (next_stage == POMODORO_FOCUS) {
        focus_count++;
    } else if (next_stage == POMODORO_SHORT_BREAK) {
        short_break_count++;
    } else if (next_stage == POMODORO_LONG_BREAK) {
        long_break_count++;
    }

    paused_from_stage = next_stage;
    current_stage = POMODORO_TRANSITION;
    start_time = millis();
    elapsed_time = 0;
    set_stage_label(next_stage);
}

void pomodoro_pause() {
    if (current_stage == POMODORO_READY ||
        current_stage == POMODORO_PAUSED ||
        current_stage == POMODORO_TRANSITION) {
        return;
    }

    paused_from_stage = current_stage;
    current_stage = POMODORO_PAUSED;
    elapsed_time += millis() - start_time;
}

extern "C" void pomodoro_toggle() {
    // Alternar entre pausar i reprendre el temporitzador Pomodoro, connectat amb el botó de la interfície d'usuari
    switch (current_stage) {
        case POMODORO_READY:
            focus_count = 1;
            short_break_count = 0;
            long_break_count = 0;
            start_stage(POMODORO_FOCUS);
            break;
        case POMODORO_FOCUS:
        case POMODORO_SHORT_BREAK:
        case POMODORO_LONG_BREAK:
            pomodoro_pause();
            break;
        case POMODORO_PAUSED:
        case POMODORO_TRANSITION:
            current_stage = paused_from_stage;
            start_time = millis();
            elapsed_time = 0;
            break;
    }
}

void pomodoro_update() {
    if (current_stage == POMODORO_PAUSED ||
        current_stage == POMODORO_READY ||
        current_stage == POMODORO_TRANSITION) {
        return;
    }

    const unsigned long elapsed = elapsed_time + (millis() - start_time);
    if (elapsed < get_stage_duration(current_stage)) return;

    if (current_stage == POMODORO_FOCUS) {
        // Si el residu és 0 --> descans llarg
        const bool is_long_break = (focus_count % CYCLES_BEFORE_LONG_BREAK) == 0;
        const PomodoroStage next_stage = is_long_break ? POMODORO_LONG_BREAK : POMODORO_SHORT_BREAK;
        transition_to_stage(next_stage);
    } else {
        transition_to_stage(POMODORO_FOCUS);
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

    if (current_stage == POMODORO_TRANSITION) {
        const unsigned long duration = get_stage_duration(paused_from_stage);
        return duration / 1000 / 60;
    }

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


}
