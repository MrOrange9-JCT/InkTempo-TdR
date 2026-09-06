#pragma once
#include <stdint.h>

struct StatCounter {
    uint32_t value;
    uint32_t goal;
};

void stats_init();
void stats_update(); // checks RTC, resets all counters on day change

void stats_add_focus_minutes(uint32_t minutes);
void stats_add_task_completed(bool done);

const StatCounter& stats_focus();
const StatCounter& stats_tasks();

extern "C" void stats_set_focus_goal(uint32_t minutes);
extern "C" void stats_set_tasks_goal(uint32_t count);
void stats_sync_time();

extern "C" uint32_t stats_focus_value();
extern "C" uint32_t stats_focus_goal();
extern "C" uint32_t stats_tasks_value();
extern "C" uint32_t stats_tasks_goal();