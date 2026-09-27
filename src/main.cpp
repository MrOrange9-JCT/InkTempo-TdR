#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <TouchDrvGT911.hpp>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <Preferences.h>

#include <esp_heap_caps.h>
#include <esp_sleep.h>
#include <epdiy.h>
#include <lvgl.h>

#include "lilygo_t5s3_board.h"
#include "config.h"
#include "ui/ui.h"
#include "ui/fonts.h"
#include "ui/vars.h"
#include "modules/pomodoro.h"
#include "modules/tasks.h"
#include "modules/stats.h"
#include "modules/date_format.h"

static Preferences prefs;

// Controlador tàctil
static TouchDrvGT911 touch;
static bool touch_available = false;
static bool screen_rotated = false;

// Framebuffer d'alt nivell i característiques de la pantalla
static EpdiyHighlevelState highlevel;
static uint8_t *epd_framebuffer;

static uint8_t *gray_buffer;
static volatile bool frame_pending;
static volatile bool touch_active;
static unsigned long last_touch = 0;
static bool woke_from_button = false;
static bool button_down = false;
static unsigned long button_pressed_at = 0;
static bool long_press_handled = false;
static bool ignore_button_until_release = false;
static const unsigned long LONG_PRESS_MS = 1500;
static const unsigned long ENTER_SLEEP_MS = (POMODORO_FOCUS_MINUTES * 60 * 1000) + (15 * 60 * 1000); // Temps d'inactivitat: temps de pomodoro + 15 minuts de marge
static const uint16_t display_width = 960;
static const uint16_t display_height = 540;
static const size_t gray_buffer_size = (display_width * display_height + 1) / 2;

static void refresh_pending_frame(bool allow_during_touch = false);
static void apply_pomodoro_theme(bool active);
static void sync_pomodoro_ui();
static void update_header_time();
static void task_checked_event(lv_event_t *event);
static bool pomodoro_theme_active = false;

// Full refresh per netejar ghosting
static const int FULL_REFRESH_INTERVAL_MS = 10 * 60 * 1000; // 10 minuts
static unsigned long last_full_refresh = 0;

// Estat de les pàgines
static uint8_t current_page = 1;
static lv_obj_t *previous_page_button;
static lv_obj_t *next_page_button;

String last_remaining_time_str = get_var_pomodoro_timer(); // Inicialitzar amb el valor real de la variable EEZ

// Notion
static const int NOTION_QUERY_INTERVAL_MS = 5 * 60 * 1000; // 5 minuts
static unsigned long last_notion_sync = 0;

// Bateria
static const int BATTERY_CHECK_INTERVAL_MS = 60 * 1000; // 1 minut
static unsigned long last_battery_check = 0;
int vref = 1100; // Valor de referència per a la lectura del voltatge de la bateria (en mV)
float last_battery_level = 0.0f; // Últim percentatge de la bateria llegit

static void update_header_time() {
    static char last_time[32] = "";
    struct tm timeinfo;

    if (!getLocalTime(&timeinfo, 0)) return;

    char date_text[24];
    char time_text[sizeof(last_time)];
    format_date_cat(date_text, sizeof(date_text), timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
    snprintf(time_text, sizeof(time_text), "%02d:%02d  %s", timeinfo.tm_hour, timeinfo.tm_min, date_text);
    if (strcmp(time_text, last_time) == 0) return;

    strcpy(last_time, time_text);
    set_var_current_time(time_text);
}

static void update_page_indicators() {
    // Actualitzar els punts de la part inferior segons la pàgina actual
    static const char *page_titles[] = { "ESTADÍSTIQUES", "POMODORO", "TASQUES" };
    lv_obj_t *dots[] = { objects.dot_0, objects.dot_1, objects.dot_2 };

    set_var_screen_title(page_titles[current_page]);

    for (uint8_t index = 0; index < 3; index++) {
        lv_color_t indicator_color = pomodoro_theme_active ? lv_color_hex(0xffffff) : lv_color_hex(0x000000);
        lv_obj_set_style_bg_color(dots[index], indicator_color, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(dots[index], indicator_color, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(dots[index], index == current_page ? LV_OPA_COVER : LV_OPA_TRANSP,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    tick_screen_main();
}

static void update_page_buttons() {
    // Actualitzar la visibilitat dels botons de navegació segons la pàgina actual
    if (current_page == 0) {
        lv_obj_add_flag(previous_page_button, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(next_page_button, LV_OBJ_FLAG_HIDDEN);
    } else if (current_page == 2) {
        lv_obj_clear_flag(previous_page_button, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(next_page_button, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_flag(previous_page_button, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(next_page_button, LV_OBJ_FLAG_HIDDEN);
    }
}

static void page_button_event(lv_event_t *event) {
    pomodoro_pause();
    sync_pomodoro_ui();

    // Registrar l'esddeveniment de clic del botó < o > i canviar la pàgina
    // TODO: Considerar si es pot canviar el mode d'actualització d'EPDiy per evitar ghosting
    int8_t direction = (int8_t)(intptr_t)lv_event_get_user_data(event);
    int8_t next_page = (int8_t)current_page + direction;
    if (next_page < 0 || next_page > 2) return;

    current_page = (uint8_t)next_page;
    lv_obj_t *tiles[] = { objects.obj0, objects.obj1, objects.obj2 };
    lv_obj_set_tile(objects.content, tiles[current_page], LV_ANIM_OFF);
    update_page_indicators();
    update_page_buttons();
}

static void tileview_event(lv_event_t *event) {
    if (lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED) return;

    pomodoro_pause();
    sync_pomodoro_ui();

    lv_obj_t *active_tile = lv_tileview_get_tile_act(objects.content);
    if (active_tile == objects.obj0) current_page = 0;
    else if (active_tile == objects.obj1) current_page = 1;
    else if (active_tile == objects.obj2) current_page = 2;

    update_page_indicators();
    update_page_buttons();
}

static void create_page_buttons() {
    // Crear botons de navegació a la pantalla principal

    // Botó anterior "<"
    previous_page_button = lv_btn_create(objects.main);
    lv_obj_set_pos(previous_page_button, 0, 51);
    lv_obj_set_size(previous_page_button, 64, 438);
    lv_obj_add_event_cb(previous_page_button, page_button_event, LV_EVENT_CLICKED, (void *)(intptr_t)-1);
    lv_obj_t *previous_label = lv_label_create(previous_page_button);
    lv_label_set_text(previous_label, "<");

    // Botó següent ">"
    next_page_button = lv_btn_create(objects.main);
    lv_obj_set_pos(next_page_button, 896, 51);
    lv_obj_set_size(next_page_button, 64, 438);
    lv_obj_add_event_cb(next_page_button, page_button_event, LV_EVENT_CLICKED, (void *)(intptr_t)1);
    lv_obj_t *next_label = lv_label_create(next_page_button);
    lv_label_set_text(next_label, ">");

    // Aplicar estils als botons i les etiquetes (INVISIBLE; SENSE ANIMACIÓ)
    for (lv_obj_t *btn : {previous_page_button, next_page_button}) {
        for (lv_state_t state : {LV_STATE_DEFAULT, LV_STATE_PRESSED}) {
            lv_obj_set_style_bg_color(btn, lv_color_hex(0xffffff), LV_PART_MAIN | state);
            lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, LV_PART_MAIN | state);
            lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN | state);
            lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN | state);
        }
        lv_obj_set_style_radius(btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(btn, &ui_font_inter_medium_24pt, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t *label = lv_obj_get_child(btn, 0);
        lv_obj_set_style_text_font(label, &ui_font_inter_medium_24pt, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_opa(label, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_center(label);
    }

    update_page_buttons();
    lv_obj_add_event_cb(objects.content, tileview_event, LV_EVENT_VALUE_CHANGED, NULL);
}

static lv_color_t pomodoro_theme_color(lv_color_t color, bool active) {
    bool is_black = LV_COLOR_GET_R(color) == 0 && LV_COLOR_GET_G(color) == 0 && LV_COLOR_GET_B(color) == 0;
    bool is_white = LV_COLOR_GET_R(color) == 31 && LV_COLOR_GET_G(color) == 63 && LV_COLOR_GET_B(color) == 31;

    if (active && is_white) return lv_color_hex(0x000000);
    if (active && is_black) return lv_color_hex(0xffffff);
    if (!active && is_black) return lv_color_hex(0xffffff);
    if (!active && is_white) return lv_color_hex(0x000000);
    return color;
}

static void apply_pomodoro_theme_to_object(lv_obj_t *object, bool active) {
    lv_style_selector_t selector = LV_PART_MAIN | LV_STATE_DEFAULT;
    lv_obj_set_style_bg_color(object, pomodoro_theme_color(lv_obj_get_style_bg_color(object, selector), active), selector);
    lv_obj_set_style_text_color(object, pomodoro_theme_color(lv_obj_get_style_text_color(object, selector), active), selector);
    lv_obj_set_style_border_color(object, pomodoro_theme_color(lv_obj_get_style_border_color(object, selector), active), selector);
    lv_obj_set_style_line_color(object, pomodoro_theme_color(lv_obj_get_style_line_color(object, selector), active), selector);
    lv_obj_set_style_outline_color(object, pomodoro_theme_color(lv_obj_get_style_outline_color(object, selector), active), selector);
    lv_obj_set_style_shadow_color(object, pomodoro_theme_color(lv_obj_get_style_shadow_color(object, selector), active), selector);

    for (uint32_t child_index = 0; child_index < lv_obj_get_child_cnt(object); child_index++) {
        apply_pomodoro_theme_to_object(lv_obj_get_child(object, child_index), active);
    }
}

static void apply_pomodoro_theme(bool active) {
    if (pomodoro_theme_active == active || objects.main == nullptr) return;

    apply_pomodoro_theme_to_object(objects.main, active);
    for (lv_obj_t *surface : {objects.content, objects.obj0, objects.obj1, objects.obj2}) {
        lv_obj_set_style_bg_opa(surface, active ? LV_OPA_COVER : LV_OPA_TRANSP,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    pomodoro_theme_active = active;
}

static void sync_pomodoro_ui() {
    static PomodoroStage last_stage = POMODORO_READY;
    PomodoroStage stage = pomodoro_get_stage();
    bool active = stage != POMODORO_READY && stage != POMODORO_PAUSED && stage != POMODORO_TRANSITION; // Està actiu si no està en READY ni PAUSED ni TRANSITION

    apply_pomodoro_theme(active);

    if (stage != last_stage) {
        set_var_pomodoro_stage_label(pomodoro_get_label());
        last_stage = stage;
    }

    if (stage != POMODORO_READY) {
        unsigned long remaining_minutes = pomodoro_get_remaining_minutes();
        String remaining_time_str = String(remaining_minutes) + "′";
        if (remaining_time_str != last_remaining_time_str) { // Només actualitzar si ha canviat
            if (stage == POMODORO_FOCUS) {
                stats_add_focus_minutes(1); // Afegir 1 minut a les estadístiques de concentració
            }
            last_remaining_time_str = remaining_time_str;
            set_var_pomodoro_timer(remaining_time_str.c_str());
        }
    }
}

void update_tasks_list() {
    // Actualitzar la llista de tasques a la interfície d'usuari
    static lv_obj_t *task_list = nullptr;
    if (task_list == nullptr) {
        task_list = lv_obj_get_child(objects.obj2, 1);
        lv_obj_set_scroll_dir(task_list, LV_DIR_VER);
        lv_obj_set_scrollbar_mode(task_list, LV_SCROLLBAR_MODE_OFF);
    }

    lv_obj_clean(task_list);

    for (int i = 0; i < get_task_count(); i++) {
        // Generar una nova fila (tasca) [CODI COPIAT DE ui/screens.c]
        NotionTask task = get_task(i);
        if (task.title.isEmpty()) continue; // Ignorar tasques sense títol

        lv_obj_t *task_row = lv_list_create(task_list);
        lv_obj_set_size(task_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_scroll_dir(task_row, LV_DIR_NONE);
        lv_obj_set_scrollbar_mode(task_row, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_style_flex_flow(task_row, LV_FLEX_FLOW_ROW, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(task_row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(task_row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_outline_opa(task_row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(task_row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(task_row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_flex_cross_place(task_row, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(task_row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t *checkbox = lv_checkbox_create(task_row);
        lv_checkbox_set_text_static(checkbox, "");
        lv_obj_set_style_border_color(checkbox, lv_color_hex(0x000000), LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_obj_set_style_transform_width(checkbox, -2, LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_obj_set_style_transform_height(checkbox, -2, LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(checkbox, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(checkbox, lv_color_hex(0xffffff), LV_PART_INDICATOR | LV_STATE_CHECKED);
        lv_obj_set_style_bg_opa(checkbox, 255, LV_PART_INDICATOR | LV_STATE_CHECKED);
        lv_obj_set_style_text_font(checkbox, &lv_font_montserrat_16, LV_PART_INDICATOR | LV_STATE_CHECKED);
        lv_obj_set_style_outline_color(checkbox, lv_color_hex(0x000000), LV_PART_INDICATOR | LV_STATE_CHECKED);
        lv_obj_set_style_border_color(checkbox, lv_color_hex(0x666666), LV_PART_INDICATOR | LV_STATE_CHECKED);
        lv_obj_set_style_bg_color(checkbox, lv_color_hex(0x666666), LV_PART_INDICATOR | LV_STATE_CHECKED);
        lv_obj_set_style_text_color(checkbox, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(checkbox, &ui_font_inter_medium_24pt, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(checkbox, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_outline_opa(checkbox, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(checkbox, lv_color_hex(0x666666), LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_add_event_cb(checkbox, task_checked_event, LV_EVENT_VALUE_CHANGED, nullptr);
        if (task.subject == "ERROR") {
            lv_obj_add_state(checkbox, LV_STATE_DISABLED);
            lv_obj_add_flag(checkbox, LV_OBJ_FLAG_HIDDEN);
        }
            

        lv_obj_t *subject = lv_label_create(task_row);
        lv_label_set_text(subject, task.subject.c_str());
        lv_obj_set_style_text_color(subject, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(subject, &ui_font_inter_medium_24pt, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(subject, lv_color_hex(0x555555), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(subject, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(subject, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(subject, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_right(subject, 10, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_outline_width(subject, 2, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_outline_color(subject, lv_color_hex(0x666666), LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_bg_color(subject, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_text_color(subject, lv_color_hex(0x666666), LV_PART_MAIN | LV_STATE_CHECKED);

        lv_obj_t *title = lv_label_create(task_row);
        lv_label_set_text(title, task.title.c_str());
        lv_label_set_long_mode(title, LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_color(title, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(title, &ui_font_inter_medium_24pt, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(title, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_max_width(title, 500, LV_PART_MAIN | LV_STATE_DEFAULT);
        // if checked
        lv_obj_set_style_text_decor(title, LV_TEXT_DECOR_STRIKETHROUGH, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_text_color(title, lv_color_hex(0x666666), LV_PART_MAIN | LV_STATE_CHECKED);

        lv_obj_t *deadline = lv_label_create(task_row);
        lv_label_set_text(deadline, task.deadline.c_str());
        lv_obj_set_style_text_color(deadline, lv_color_hex(0x555555), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(deadline, &ui_font_inter_medium_24pt, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_left(deadline, 8, LV_PART_MAIN | LV_STATE_DEFAULT);

        if (task.done) {
            // TODO: Actualitzar just quan es completa la tasca (des de LVGL), no només quan es carrega la llista
            lv_obj_add_state(checkbox, LV_STATE_CHECKED);
            lv_obj_add_state(title, LV_STATE_CHECKED);
            lv_obj_add_state(subject, LV_STATE_CHECKED);
        }

        // Reaplicar tema No-Pomodoro (evitar text blanc sobre blanc)
        //apply_pomodoro_theme_to_object(task_row, !pomodoro_theme_active);
    }
}

static void task_checked_event(lv_event_t *event) {
    lv_obj_t *checkbox = lv_event_get_target(event);
    bool checked = lv_obj_has_state(checkbox, LV_STATE_CHECKED);

    // Obtenir el títol de la tasca associada al checkbox
    lv_obj_t *task_row = lv_obj_get_parent(checkbox);
    lv_obj_t *title_label = lv_obj_get_child(task_row, 2);
    const char *task_title = lv_label_get_text(title_label);

    // Actualitzar l'estat de la tasca a Notion (aquí només es mostra un missatge)
    Serial.printf("[Notion] Tasca (%d) '%s' marcada com %s.\n", lv_obj_get_index(task_row), task_title, checked ? "completa" : "incompleta");

    lv_timer_handler();
    refresh_pending_frame(true);

    mark_task_done(lv_obj_get_index(task_row), checked);

    query_notion(); // Actualitzar la llista de tasques després de marcar com a completada/incompleta
    update_tasks_list();

    // TODO: Millorar actualització amb un altre mode de refresh, hi ha molt de ghosting amb el mode actual (MODE_GL16)
}

void update_stats_screen() {
    // Actualitzar la pantalla d'estadístiques amb les dades actuals
    // S'haurà de canviar a EEZ Studio per poder establir les variables (vars.c)
}

static void lvgl_touch_read(lv_indev_drv_t *driver, lv_indev_data_t *data) {
    (void)driver;

    int16_t touch_x[1], touch_y[1];
    uint8_t touched = touch_available ? touch.getPoint(touch_x, touch_y, 1) : 0;
    if (touched > 0) {
        // GT911 is portrait-oriented (540x960), while LVGL is landscape (960x540).
        data->point.x = touch_y[0];
        data->point.y = (display_height - 1) - touch_x[0];
        data->state = LV_INDEV_STATE_PR;
        touch_active = true;
        last_touch = millis();
        Serial.printf("[Touch] Touch (%d, %d) -> LVGL (%d, %d)\n",
                      touch_x[0], touch_y[0], data->point.x, data->point.y);
    } else {
        data->state = LV_INDEV_STATE_REL;
        touch_active = false;
    }
}

static uint8_t color_to_gray(lv_color_t color) {
    uint8_t red = (uint8_t)((LV_COLOR_GET_R(color) * 255U) / 31U);
    uint8_t green = (uint8_t)((LV_COLOR_GET_G(color) * 255U) / 63U);
    uint8_t blue = (uint8_t)((LV_COLOR_GET_B(color) * 255U) / 31U);
    uint8_t luminance = (uint8_t)((77U * red + 150U * green + 29U * blue) >> 8);
    return luminance >> 4;
}

static void flush_to_epdiy(lv_disp_drv_t *display, const lv_area_t *area, lv_color_t *colors) {
    uint16_t width = lv_area_get_width(area);
    uint16_t height = lv_area_get_height(area);
    uint16_t row_bytes = (display_width + 1) / 2;

    for (uint16_t y = 0; y < height; y++) {
        for (uint16_t x = 0; x < width; x++) {
            uint16_t display_x = (uint16_t)(area->x1 + x);
            uint16_t display_y = (uint16_t)(area->y1 + y);
            uint8_t *packed = &gray_buffer[display_y * row_bytes + display_x / 2];
            uint8_t gray = color_to_gray(colors[y * width + x]);

            if ((display_x & 1U) == 0) {
                *packed = (uint8_t)((*packed & 0xF0U) | gray);
            } else {
                *packed = (uint8_t)((*packed & 0x0FU) | (gray << 4));
            }
        }
    }

    frame_pending = true;
    lv_disp_flush_ready(display);
}

static void refresh_pending_frame(bool allow_during_touch) {
    if (!frame_pending || (touch_active && !allow_during_touch)) return;

    EpdRect full_area = { .x = 0, .y = 0, .width = display_width, .height = display_height };
    frame_pending = false;
    epd_draw_rotated_image(full_area, gray_buffer, epd_framebuffer);
    epd_poweron();
    epd_hl_update_area(&highlevel, MODE_GL16, epd_ambient_temperature(), full_area);
    epd_poweroff();
}

static float read_battery_level() {
    // Llegir el percentatge de la bateria a partir del voltatge mesurat
    const float min_voltage = 3.0f;
    const float max_voltage = 4.2f;

    uint16_t v = analogRead(LILYGO_T5S3_BATT_ADC);
    float battery_voltage = ((float)v / 4095.0f) * 2.0f * 3.3f * (vref / 1000.0f);
    float battery_percentage = (battery_voltage - min_voltage) / (max_voltage - min_voltage) * 100.0f;

    return constrain(battery_percentage, 0.0f, 100.0f);
}

static void lvgl_display_init() {
    lv_init();

    static lv_disp_draw_buf_t draw_buffer;
    lv_color_t *buffer_1 = (lv_color_t *)heap_caps_malloc(
        display_width * display_height * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    lv_color_t *buffer_2 = (lv_color_t *)heap_caps_malloc(
        display_width * display_height * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    gray_buffer = (uint8_t *)heap_caps_malloc(gray_buffer_size, MALLOC_CAP_SPIRAM);

    lv_disp_draw_buf_init(&draw_buffer, buffer_1, buffer_2, display_width * display_height);

    static lv_disp_drv_t display;
    lv_disp_drv_init(&display);
    display.hor_res = display_width;
    display.ver_res = display_height;
    display.flush_cb = flush_to_epdiy;
    display.draw_buf = &draw_buffer;
    display.full_refresh = 0;
    lv_disp_drv_register(&display);

    static lv_indev_drv_t touch_driver;
    lv_indev_drv_init(&touch_driver);
    touch_driver.type = LV_INDEV_TYPE_POINTER;
    touch_driver.read_cb = lvgl_touch_read;
    lv_indev_drv_register(&touch_driver);

    memset(gray_buffer, 0xFF, gray_buffer_size);
}

static void enter_sleep_mode() {
    Serial.println("[System] Entrant en mode de suspensió (sleep mode)...");
    epd_poweron();
    epd_fullclear(&highlevel, 25);
    epd_poweroff();

    delay(1000);

    WiFi.disconnect(true);

    delay(100);

    Wire.end();
    Serial.end();

    esp_sleep_enable_ext1_wakeup(1ULL << LILYGO_T5S3_BUTTON_1, ESP_EXT1_WAKEUP_ANY_LOW);
    esp_deep_sleep_start();
}

static void rotate_screen() {
    // Girar la pantalla 180 graus (per a LilyGo T5-S3 amb pantalla ED047TC1)
    if (screen_rotated) {
        Serial.println("[System] Girant pantalla a la posició normal...");
        epd_set_rotation(EPD_ROT_LANDSCAPE);

        if (touch_available) {
            touch.setMaxCoordinates(display_width - 1, display_height - 1);
            touch.setMirrorXY(false, false);
        }

        screen_rotated = false;
    } else {
        Serial.println("[System] Girant pantalla a la posició invertida...");
        epd_set_rotation(EPD_ROT_INVERTED_LANDSCAPE);

        if (touch_available) {
            touch.setMaxCoordinates(display_height - 1, display_width - 1);
            touch.setMirrorXY(true, true);
        }

        screen_rotated = true;
    }

    prefs.begin("config", false);
    prefs.putUInt("rotation", screen_rotated ? EPD_ROT_INVERTED_LANDSCAPE : EPD_ROT_LANDSCAPE);
    prefs.end();

    frame_pending = true;
    lv_refr_now(lv_disp_get_default());
}

static void handle_button() {
    bool is_down = digitalRead(LILYGO_T5S3_BUTTON_1) == LOW;

    if (!is_down && ignore_button_until_release) {
        ignore_button_until_release = false;
        button_down = false;
        return;
    }

    if (is_down && !button_down) {
        button_down = true;
        button_pressed_at = millis();
        long_press_handled = false;
    }

    if (is_down && button_down && !ignore_button_until_release && !long_press_handled &&
        millis() - button_pressed_at >= LONG_PRESS_MS) {
        Serial.println("[System] Botó 1 mantingut premut; entrant en suspensió...");
        long_press_handled = true;
        enter_sleep_mode();
    }

    if (!is_down && button_down) {
        button_down = false;
        if (ignore_button_until_release) {
            ignore_button_until_release = false;
        } else if (!long_press_handled) {
            Serial.println("[System] Polsació curta del botó 1; girant pantalla 180 graus...");
            rotate_screen();
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    esp_sleep_wakeup_cause_t wake_cause = esp_sleep_get_wakeup_cause();
    woke_from_button = wake_cause == ESP_SLEEP_WAKEUP_EXT1;
    if (woke_from_button) {
        Serial.println("[System] Despertat pel boto; esperant que es deixi anar...");
    }

    Serial.println("\n=========================================");
    Serial.println("      TREC - Jordi Castro - 2026-27");
    Serial.println("=========================================");


    // 1. Inicialitzar botó1 i pantalla tàctil
    pinMode(LILYGO_T5S3_BUTTON_1, INPUT_PULLUP);
    last_touch = millis();
    ignore_button_until_release = woke_from_button;

    Wire.begin(LILYGO_T5S3_TOUCH_SDA, LILYGO_T5S3_TOUCH_SCL);
    touch.setPins(-1, LILYGO_T5S3_TOUCH_INT);
    if (touch.begin(Wire, 0x14, LILYGO_T5S3_TOUCH_SDA, LILYGO_T5S3_TOUCH_SCL) ||
        touch.begin(Wire, 0x5D, LILYGO_T5S3_TOUCH_SDA, LILYGO_T5S3_TOUCH_SCL)) {
        touch_available = true;
        Serial.println("[Touch] Pantalla tàctil GT911 inciciada correctament.");
    } else {
        Serial.println("[Touch] Pantalla tàctil GT911 no detectada (Botó 21 i Serial actius).");
    }

    // Inicialitzar WiFi i Notion
    connect_wifi();
    query_notion();

    // 2. Inicialitzar EPDiy per a LilyGo T5-S3 amb pantalla ED047TC1
    Serial.println("[EPDiy] Iniciant EPDiy per a LilyGo T5-S3 amb pantalla ED047TC1...");
    epd_init(&lilygo_t5s3_board, &ED047TC1, EPD_LUT_64K);
    highlevel = epd_hl_init(EPD_BUILTIN_WAVEFORM);

    // 3. Inicialitzar Dual Framebuffer + Diff State
    Serial.println("[EPDiy] Assignant buffers de framebuffer a PSRAM...");
    highlevel.mirror_x = false;
    epd_framebuffer = epd_hl_get_framebuffer(&highlevel);

    // Iniciar RTC i comptadors d'estadístiques
    stats_init();

    // 4. Netejar pantalla
    Serial.println("[EPDiy] Netejant pantalla inicial...");
    epd_poweron();
    epd_fullclear(&highlevel, 25);
    epd_poweroff();

    // 5. Inicialitzar LVGL i interfície d'usuari
    lvgl_display_init();
    epd_framebuffer = epd_hl_get_framebuffer(&highlevel);
    ui_init();
    create_page_buttons();
    update_tasks_list(); // TAMBÉ INICIA WIFI
    update_header_time();
    update_page_indicators();

    // 6. Aplicar rotació de pantalla si està configurada a les preferències
    prefs.begin("config", false);
    static EpdRotation rotation = static_cast<EpdRotation>(prefs.getUInt("rotation", 0));
    if (rotation == EPD_ROT_INVERTED_LANDSCAPE) {
        rotate_screen(); // També fa el primer lv_refr_now()
    }
    prefs.end();

    refresh_pending_frame();
}

void loop() {
    // Actualitzar l'estat del temporitzador Pomodoro i la interfície d'usuari
    pomodoro_update();
    sync_pomodoro_ui();
    update_header_time();

    // Sincronitzar tasques des de Notion cada x temps
    // TODO: En comptes de fer-ho cada 5 minuts, fer-ho només quan es canvia alguna cosa a Notion. Revisar API.
    if (millis() - last_notion_sync >= NOTION_QUERY_INTERVAL_MS) {
        last_notion_sync = millis();
        query_notion();
        update_tasks_list();
    }

    // Refrescar la pantalla (full refresh) per netejar ghosting
    if (millis() - last_full_refresh >= FULL_REFRESH_INTERVAL_MS) {
        Serial.println("[EPDiy] Realitzant full refresh per netejar ghosting...");
        last_full_refresh = millis();
        epd_poweron();
        epd_fullclear(&highlevel, 25);
        epd_poweroff();

        frame_pending = true;
        refresh_pending_frame();
    }

    // Nivell de la bateria (en %)
    if (millis() - last_battery_check >= BATTERY_CHECK_INTERVAL_MS) {
        last_battery_check = millis();
        float battery_percentage = read_battery_level();
        if (last_battery_level == 0.0f || (last_battery_level - battery_percentage) >= 1.0f || (last_battery_level != battery_percentage) && last_battery_level == 100.0f || battery_percentage == 100.0f) {
            char battery_level_text[12];
            snprintf(battery_level_text, sizeof(battery_level_text), "%d%%", (int)round(battery_percentage));

            Serial.print("[System] Nivell de bateria: ");
            Serial.println(battery_level_text);

            set_var_battery_level(battery_level_text);
            last_battery_level = battery_percentage;
        }
    }

    if (millis() - last_touch >= ENTER_SLEEP_MS ||
        false) {
        Serial.println("[System] Mode de suspensió activat per inactivitat...");
        enter_sleep_mode();
    }

    handle_button();

    ui_tick();
    lv_timer_handler();
    refresh_pending_frame();

    delay(50);
}