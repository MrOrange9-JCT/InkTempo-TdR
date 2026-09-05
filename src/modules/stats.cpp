#include "stats.h"
#include "time.h"
#include <Wire.h>
#include "SensorPCF8563.hpp"
#include "lilygo_t5s3_board.h"

static StatCounter focus_counter = {0, 90}; // objectiu de 90 min 
static StatCounter tasks_counter = {0, 5}; // objectiu de 5 tasques 

static SensorPCF8563 rtc;
static bool rtc_available = false;

static uint16_t last_year = 0;
static uint8_t last_month = 0, last_day = 0;

// void stats_sync_time();

static void reset_counters() {
    focus_counter.value = 0;
    tasks_counter.value = 0;
}

void stats_init() {
    if (!rtc.begin(Wire, LILYGO_T5S3_TOUCH_SDA, LILYGO_T5S3_TOUCH_SCL)) {
        Serial.println("[Stats] Error: No es pot inicialitzar el sensor RTC PCF8563");
        rtc_available = false;
        return;
    }
    rtc_available = true;

    stats_sync_time(); 

    RTC_DateTime datetime = rtc.getDateTime();
    last_year = datetime.getYear();
    last_month = datetime.getMonth();
    last_day = datetime.getDay();

    Serial.printf("[Stats] Inicialitzat amb data %04d-%02d-%02d\n", datetime.getYear(), datetime.getMonth(), datetime.getDay());
}

void stats_sync_time() {
    if (!rtc_available) return;

    configTime(3600, 3600, "pool.ntp.org");
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 10000)) {
        Serial.println("[Stats] No s'ha pogut sincronitzar amb NTP");
        return;
    }

    rtc.setDateTime(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

    last_year = timeinfo.tm_year + 1900;
    last_month = timeinfo.tm_mon + 1;
    last_day = timeinfo.tm_mday;

    Serial.printf("[Stats] RTC sincronitzat via NTP: %04d-%02d-%02d %02d:%02d:%02d\n",
                  last_year, last_month, last_day, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
}

void stats_update() {
    if (!rtc_available) return;

    RTC_DateTime datetime = rtc.getDateTime();

    if (datetime.getDay() != last_day || datetime.getMonth() != last_month || datetime.getYear() != last_year) {
        last_day = datetime.getDay(); last_month = datetime.getMonth(); last_year = datetime.getYear();
        reset_counters();
        Serial.printf("[Stats] Dia canviat a %04d-%02d-%02d, reiniciant comptadors\n", datetime.getYear(), datetime.getMonth(), datetime.getDay());
    }
}

void stats_add_focus_minutes(uint32_t minutes) { focus_counter.value += minutes; }
void stats_add_task_completed()                { tasks_counter.value += 1; }

const StatCounter& stats_focus() { return focus_counter; }
const StatCounter& stats_tasks() { return tasks_counter; }

// void stats_set_focus_goal(uint32_t minutes) { focus_counter.goal = minutes; }
// void stats_set_tasks_goal(uint32_t count)   { tasks_counter.goal = count; }