#include "actions.h"

extern void pomodoro_toggle(void);

void action_toggle_pomodoro(lv_event_t *e) {
    (void)e;
    pomodoro_toggle();
}
