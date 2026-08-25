#include <Arduino.h>
#include <Wire.h>
#include <TouchDrvGT911.hpp>

#include <esp_heap_caps.h>
#include <epdiy.h>
#include <lvgl.h>

#include "lilygo_t5s3_board.h"
#include "ui/ui.h"
#include "ui/fonts.h"

// Controlador tàctil
static TouchDrvGT911 touch;
static bool touch_available = false;

// Framebuffer d'alta nivell i estat de la pantalla
static EpdiyHighlevelState highlevel;
static uint8_t *epd_framebuffer;

static uint8_t *gray_buffer;
static volatile bool frame_pending;
static volatile bool touch_active;
static const uint16_t display_width = 960;
static const uint16_t display_height = 540;
static const size_t gray_buffer_size = (display_width * display_height + 1) / 2;
static uint8_t current_page = 1;
static lv_obj_t *previous_page_button;
static lv_obj_t *next_page_button;

static void update_page_buttons() {
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
    // Registrar l'event de clic del botó < o > i canviar la pàgina
    int8_t direction = (int8_t)(intptr_t)lv_event_get_user_data(event);
    int8_t next_page = (int8_t)current_page + direction;
    if (next_page < 0 || next_page > 2) return;

    current_page = (uint8_t)next_page;
    lv_obj_t *tiles[] = { objects.obj0, objects.obj1, objects.obj2 };
    lv_obj_set_tile(objects.content, tiles[current_page], LV_ANIM_OFF);
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

static void refresh_pending_frame() {
    if (!frame_pending || touch_active) return;

    EpdRect full_area = { .x = 0, .y = 0, .width = display_width, .height = display_height };
    frame_pending = false;
    epd_draw_rotated_image(full_area, gray_buffer, epd_framebuffer);
    epd_poweron();
    epd_hl_update_area(&highlevel, MODE_GL16, epd_ambient_temperature(), full_area);
    epd_poweroff();
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

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n==========================================");
    Serial.println("      TREC - Jordi Castro - 2026-27");
    Serial.println("==========================================");

    // 1. Inicialitzar botó1 i pantalla tàctil
    pinMode(LILYGO_T5S3_BUTTON_1, INPUT_PULLUP);

    Wire.begin(LILYGO_T5S3_TOUCH_SDA, LILYGO_T5S3_TOUCH_SCL);
    touch.setPins(-1, LILYGO_T5S3_TOUCH_INT);
    if (touch.begin(Wire, 0x14, LILYGO_T5S3_TOUCH_SDA, LILYGO_T5S3_TOUCH_SCL) ||
        touch.begin(Wire, 0x5D, LILYGO_T5S3_TOUCH_SDA, LILYGO_T5S3_TOUCH_SCL)) {
        touch_available = true;
        Serial.println("[Touch] Pantalla tàctil GT911 inciciada correctament.");
    } else {
        Serial.println("[Touch] Pantalla tàctil GT911 no detectada (Botó 21 i Serial actius).");
    }

    // 2. Inicialitzar EPDiy per a LilyGo T5-S3 amb pantalla ED047TC1
    Serial.println("[EPDiy] Iniciant EPDiy per a LilyGo T5-S3 amb pantalla ED047TC1...");
    epd_init(&lilygo_t5s3_board, &ED047TC1, EPD_LUT_64K);
    highlevel = epd_hl_init(EPD_BUILTIN_WAVEFORM);
    epd_set_rotation(EPD_ROT_LANDSCAPE);

    // 3. Inicialitzar Dual Framebuffer + Diff State
    Serial.println("[EPDiy] Assignant buffers de framebuffer a PSRAM...");
    highlevel.mirror_x = false;
    epd_framebuffer = epd_hl_get_framebuffer(&highlevel);

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
    lv_refr_now(lv_disp_get_default());
    refresh_pending_frame();
}

void loop() {
    ui_tick();
    lv_timer_handler();
    refresh_pending_frame();
    delay(5);
}