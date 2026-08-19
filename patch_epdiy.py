"""
PlatformIO Pre-build Script: Automatic LilyGo T5-S3 Hardware Patch for EPDiy
Automatically patches upstream EPDiy to support LilyGo T5-S3 74HC4094 shift register,
Intel 8080 parallel LCD bus, and RMT pulse timing on ESP32-S3.
"""

import os
import sys

Import("env")

I2S_DATA_BUS_C = r'''#include "i2s_data_bus.h"

#include <esp_log.h>
#include <esp_idf_version.h>
#include <esp_heap_caps.h>
#include <string.h>

#if defined(CONFIG_IDF_TARGET_ESP32S3)

#include "esp_lcd_panel_io.h"

static const char* TAG = "i80_bus";
static esp_lcd_panel_io_handle_t io_handle = NULL;
static volatile bool output_done = true;
static uint8_t* buffer = NULL;
static uint32_t line_bytes = (960 + 32) / 4;

static bool notify_trans_done(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx) {
    (void)panel_io;
    (void)edata;
    (void)user_ctx;
    output_done = true;
    return true;
}

void i2s_bus_init(i2s_bus_config* cfg, uint32_t epd_row_width) {
    line_bytes = (epd_row_width + 32) / 4;

    if (buffer == NULL) {
        buffer = (uint8_t*)heap_caps_calloc(1, 1024, MALLOC_CAP_DMA);
    }
    output_done = true;

    ESP_LOGI(TAG, "Initializing Intel 8080 bus for LilyGo T5-S3 (line_bytes=%u)...", (unsigned int)line_bytes);

    esp_lcd_i80_bus_handle_t i80_bus = NULL;
    esp_lcd_i80_bus_config_t bus_config = {
        .dc_gpio_num = cfg->start_pulse,
        .wr_gpio_num = cfg->clock,
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
        .clk_src = LCD_CLK_SRC_DEFAULT,
#endif
        .data_gpio_nums = {
            cfg->data_6,
            cfg->data_7,
            cfg->data_4,
            cfg->data_5,
            cfg->data_2,
            cfg->data_3,
            cfg->data_0,
            cfg->data_1,
        },
        .bus_width = 8,
        .max_transfer_bytes = 20 * 1024,
    };
    ESP_ERROR_CHECK(esp_lcd_new_i80_bus(&bus_config, &i80_bus));

    esp_lcd_panel_io_i80_config_t io_config = {
        .cs_gpio_num = -1,
        .pclk_hz = 10 * 1000 * 1000, // 10MHz
        .trans_queue_depth = 10,
        .dc_levels = {
            .dc_idle_level = 0,
            .dc_cmd_level = 1,
            .dc_dummy_level = 0,
            .dc_data_level = 0,
        },
        .on_color_trans_done = notify_trans_done,
        .user_ctx = NULL,
        .lcd_cmd_bits = 10,
        .lcd_param_bits = 0,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i80(i80_bus, &io_config, &io_handle));
}

void i2s_gpio_attach(i2s_bus_config* cfg) { (void)cfg; }
void i2s_gpio_detach(i2s_bus_config* cfg) { (void)cfg; }

uint8_t* i2s_get_current_buffer() {
    return buffer;
}

void i2s_switch_buffer() {}

void IRAM_ATTR i2s_start_line_output() {
    output_done = false;
    esp_lcd_panel_io_tx_color(io_handle, 0, buffer, line_bytes);
}

bool IRAM_ATTR i2s_is_busy() {
    return !output_done;
}

void i2s_bus_deinit() {
    if (buffer) { heap_caps_free(buffer); buffer = NULL; }
}

#elif defined(CONFIG_IDF_TARGET_ESP32)

void i2s_bus_init(i2s_bus_config* cfg, uint32_t epd_row_width) { (void)cfg; (void)epd_row_width; }
void i2s_gpio_attach(i2s_bus_config* cfg) { (void)cfg; }
void i2s_gpio_detach(i2s_bus_config* cfg) { (void)cfg; }
uint8_t* i2s_get_current_buffer() { return NULL; }
void i2s_switch_buffer() {}
void i2s_start_line_output() {}
bool i2s_is_busy() { return false; }
void i2s_bus_deinit() {}

#endif
'''

I2S_DATA_BUS_H = r'''#pragma once

#include "sdkconfig.h"
#include <driver/gpio.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    gpio_num_t data_0;
    gpio_num_t data_1;
    gpio_num_t data_2;
    gpio_num_t data_3;
    gpio_num_t data_4;
    gpio_num_t data_5;
    gpio_num_t data_6;
    gpio_num_t data_7;
    gpio_num_t clock;
    gpio_num_t start_pulse;
} i2s_bus_config;

void i2s_bus_init(i2s_bus_config* cfg, uint32_t epd_row_width);
void i2s_gpio_attach(i2s_bus_config* cfg);
void i2s_gpio_detach(i2s_bus_config* cfg);
void i2s_bus_deinit();
uint8_t* i2s_get_current_buffer();
void i2s_switch_buffer();
void i2s_start_line_output();
bool i2s_is_busy();

#ifdef __cplusplus
}
#endif
'''

RMT_PULSE_C = r'''#include "rmt_pulse.h"
#include <driver/rmt.h>
#include <esp_log.h>

static rmt_config_t row_rmt_config;

void rmt_pulse_init(gpio_num_t pin) {
    row_rmt_config.rmt_mode = RMT_MODE_TX;
    row_rmt_config.channel = RMT_CHANNEL_1;
    row_rmt_config.gpio_num = pin;
    row_rmt_config.mem_block_num = 2;
    row_rmt_config.clk_div = 8;
    row_rmt_config.tx_config.loop_en = false;
    row_rmt_config.tx_config.carrier_en = false;
    row_rmt_config.tx_config.carrier_level = RMT_CARRIER_LEVEL_LOW;
    row_rmt_config.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
    row_rmt_config.tx_config.idle_output_en = true;

    rmt_config(&row_rmt_config);
    rmt_driver_install(RMT_CHANNEL_1, 0, 0);
}

void rmt_pulse_deinit() {
    rmt_driver_uninstall(RMT_CHANNEL_1);
}

void IRAM_ATTR pulse_ckv_ticks(uint16_t high_time_ticks, uint16_t low_time_ticks, bool wait) {
    rmt_item32_t rmt_mem_ptr;
    if (high_time_ticks > 0) {
        rmt_mem_ptr.level0 = 1;
        rmt_mem_ptr.duration0 = high_time_ticks;
        rmt_mem_ptr.level1 = 0;
        rmt_mem_ptr.duration1 = low_time_ticks > 0 ? low_time_ticks : 1;
    } else {
        rmt_mem_ptr.level0 = 1;
        rmt_mem_ptr.duration0 = low_time_ticks;
        rmt_mem_ptr.level1 = 0;
        rmt_mem_ptr.duration1 = 0;
    }
    rmt_write_items(row_rmt_config.channel, &rmt_mem_ptr, 1, wait);
}

void IRAM_ATTR pulse_ckv_us(uint16_t high_time_us, uint16_t low_time_us, bool wait) {
    pulse_ckv_ticks(10 * high_time_us, 10 * low_time_us, wait);
}

bool IRAM_ATTR rmt_busy() {
    return false;
}
'''

RENDER_METHOD_H = r'''#pragma once

#include "sdkconfig.h"

#if !defined(RENDER_METHOD_LCD) && !defined(RENDER_METHOD_I2S)
#define RENDER_METHOD_I2S 1
#endif

enum EpdRenderMethod {
    EPD_METHOD_I2S = 1,
    EPD_METHOD_LCD = 2,
};

extern const enum EpdRenderMethod EPD_CURRENT_RENDER_METHOD;

#ifdef __clang__
#define IRAM_ATTR
void __assert_func(const char* file, int line, const char* func, const char* failedexpr);
#endif
'''

RENDER_METHOD_C = r'''#include "render_method.h"

#ifdef RENDER_METHOD_LCD
const enum EpdRenderMethod EPD_CURRENT_RENDER_METHOD = EPD_METHOD_LCD;
#else
const enum EpdRenderMethod EPD_CURRENT_RENDER_METHOD = EPD_METHOD_I2S;
#endif
'''

EPD_INIT_CONFIG_H = r'''#pragma once

typedef struct EpdI2cConfig {
    int port;
} EpdI2cConfig;

typedef struct EpdInitConfig {
    const EpdI2cConfig* i2c;
} EpdInitConfig;
'''

EPD_BOARD_COMMON_C = r'''#include "epd_board.h"

void epd_board_temperature_init_v2() {}
float epd_board_ambient_temperature_v2() { return 22.0f; }
void epd_board_temperature_deinit_v2() {}
'''

def patch_epdiy_source(epdiy_dir):
    print(f"\n[Patch] Applying LilyGo T5-S3 driver patch to: {epdiy_dir}")
    
    # 1. i2s_data_bus.c
    with open(os.path.join(epdiy_dir, "src", "output_i2s", "i2s_data_bus.c"), "w", encoding="utf-8") as f:
        f.write(I2S_DATA_BUS_C)
        
    # 2. i2s_data_bus.h
    with open(os.path.join(epdiy_dir, "src", "output_i2s", "i2s_data_bus.h"), "w", encoding="utf-8") as f:
        f.write(I2S_DATA_BUS_H)

    # 3. rmt_pulse.c
    with open(os.path.join(epdiy_dir, "src", "output_i2s", "rmt_pulse.c"), "w", encoding="utf-8") as f:
        f.write(RMT_PULSE_C)

    # 4. render_method.h & render_method.c
    with open(os.path.join(epdiy_dir, "src", "output_common", "render_method.h"), "w", encoding="utf-8") as f:
        f.write(RENDER_METHOD_H)
    with open(os.path.join(epdiy_dir, "src", "output_common", "render_method.c"), "w", encoding="utf-8") as f:
        f.write(RENDER_METHOD_C)

    # 5. epd_init_config.h
    with open(os.path.join(epdiy_dir, "src", "epd_init_config.h"), "w", encoding="utf-8") as f:
        f.write(EPD_INIT_CONFIG_H)

    # 6. epd_board_common.c
    with open(os.path.join(epdiy_dir, "src", "board", "epd_board_common.c"), "w", encoding="utf-8") as f:
        f.write(EPD_BOARD_COMMON_C)

    # 7. Stub unused board files (EPDiy v6/v7 hardware specific)
    for bfile in ["epd_board_v6.c", "epd_board_v7.c", "epd_board_v7_103.c", "epd_board_v7_raw.c", "pca9555.c", "tps65185.c", "epd_board_i2c.c", "lilygo_board_s3.c"]:
        bpath = os.path.join(epdiy_dir, "src", "board", bfile)
        if os.path.exists(bpath):
            with open(bpath, "w", encoding="utf-8") as f:
                f.write("// Stubbed for LilyGo T5-S3 on Arduino 2.x\n")

    # Stub epd_board_i2c.h, pca9555.h, tps65185.h
    for hfile in ["epd_board_i2c.h", "pca9555.h", "tps65185.h"]:
        hpath = os.path.join(epdiy_dir, "src", "board", hfile)
        if os.path.exists(hpath):
            with open(hpath, "w", encoding="utf-8") as f:
                f.write("#pragma once\n// Stubbed for LilyGo T5-S3\n")

    # Stub output_lcd driver files
    for lfile in ["lcd_driver.c", "render_lcd.c"]:
        lpath = os.path.join(epdiy_dir, "src", "output_lcd", lfile)
        if os.path.exists(lpath):
            with open(lpath, "w", encoding="utf-8") as f:
                f.write("// Stubbed for LilyGo T5-S3\n")

    # 8. rmt_compat.c stub for Arduino 2.x
    rmt_compat_path = os.path.join(epdiy_dir, "src", "output_common", "rmt_compat.c")
    if os.path.exists(rmt_compat_path):
        with open(rmt_compat_path, "w", encoding="utf-8") as f:
            f.write("// Stubbed for Arduino 2.x\n")

    # 9. render_i2s.c - fix frame timing & disable reorder_line_buffer on S3
    render_i2s_path = os.path.join(epdiy_dir, "src", "output_i2s", "render_i2s.c")
    if os.path.exists(render_i2s_path):
        with open(render_i2s_path, "r", encoding="utf-8") as f:
            content = f.read()

        old_start_frame = """static void i2s_start_frame() {"""
        new_start_frame = """static void busy_delay(uint32_t cycles) {
    volatile uint64_t counts = XTHAL_GET_CCOUNT() + cycles;
    while (XTHAL_GET_CCOUNT() < counts) ;
}

static void i2s_start_frame() {"""
        if "static void busy_delay" not in content:
            content = content.replace(old_start_frame, new_start_frame, 1)

        content = content.replace(
            "pulse_ckv_us(1000, 100, false);",
            "busy_delay(240);\n    pulse_ckv_us(10, 10, false);"
        )

        content = content.replace(
            "reorder_line_buffer((uint32_t*)i2s_get_current_buffer(), ctx->display_width / 4);",
            "#ifndef CONFIG_IDF_TARGET_ESP32S3\n        reorder_line_buffer((uint32_t*)i2s_get_current_buffer(), ctx->display_width / 4);\n#endif"
        )
        content = content.replace(
            "reorder_line_buffer((uint32_t*)row, line_bytes);",
            "#ifndef CONFIG_IDF_TARGET_ESP32S3\n    reorder_line_buffer((uint32_t*)row, line_bytes);\n#endif"
        )

        with open(render_i2s_path, "w", encoding="utf-8") as f:
            f.write(content)

    # 10. lut.c - fix RENDER_METHOD_LCD enum check
    lut_c_path = os.path.join(epdiy_dir, "src", "output_common", "lut.c")
    if os.path.exists(lut_c_path):
        with open(lut_c_path, "r", encoding="utf-8") as f:
            lut_content = f.read()
        lut_content = lut_content.replace(
            "EPD_CURRENT_RENDER_METHOD == RENDER_METHOD_LCD",
            "EPD_CURRENT_RENDER_METHOD == EPD_METHOD_LCD"
        )
        with open(lut_c_path, "w", encoding="utf-8") as f:
            f.write(lut_content)

    # 11. Fix font.c miniz header for ESP32/ESP32-S3 ROM
    font_c_path = os.path.join(epdiy_dir, "src", "font.c")
    if os.path.exists(font_c_path):
        with open(font_c_path, "r", encoding="utf-8") as f:
            fcontent = f.read()
        fcontent = fcontent.replace("#include <miniz.h>", "#include <rom/miniz.h>")
        with open(font_c_path, "w", encoding="utf-8") as f:
            f.write(fcontent)

    # Mark as patched
    with open(os.path.join(epdiy_dir, ".lilygo_t5s3_patched_v8"), "w") as f:
        f.write("patched\n")
    print("[Patch] LilyGo T5-S3 patch successfully applied.\n")

def check_and_patch(*args, **kwargs):
    libdeps_dir = env.subst("$PROJECT_LIBDEPS_DIR")
    env_name = env.subst("$PIOENV")
    epdiy_dir = os.path.join(libdeps_dir, env_name, "epdiy")
    
    if os.path.exists(epdiy_dir) and not os.path.exists(os.path.join(epdiy_dir, ".lilygo_t5s3_patched_v8")):
        patch_epdiy_source(epdiy_dir)

# Hook into build system
check_and_patch()
env.AddPreAction("checkprogsize", check_and_patch)
