#include <Arduino.h>
#include <esp_heap_caps.h>
#include <epdiy.h>
#include <lvgl.h>

#include "lilygo_t5s3_board.h"
#include "ui/ui.h"

static EpdiyHighlevelState highlevel;
static uint8_t *epd_framebuffer;
static uint8_t *gray_buffer;
static volatile bool frame_pending;
static const uint16_t display_width = 960;
static const uint16_t display_height = 540;
static const size_t gray_buffer_size = (display_width * display_height + 1) / 2;

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
    if (!frame_pending) return;

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

    memset(gray_buffer, 0xFF, gray_buffer_size);
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    epd_init(&lilygo_t5s3_board, &ED047TC1, EPD_LUT_64K);
    highlevel = epd_hl_init(EPD_BUILTIN_WAVEFORM);
    epd_set_rotation(EPD_ROT_LANDSCAPE);

    epd_poweron();
    epd_clear();
    epd_poweroff();

    lvgl_display_init();
    epd_framebuffer = epd_hl_get_framebuffer(&highlevel);
    ui_init();
    lv_refr_now(lv_disp_get_default());
    refresh_pending_frame();
}

void loop() {
    ui_tick();
    lv_timer_handler();
    refresh_pending_frame();
    delay(5);
}