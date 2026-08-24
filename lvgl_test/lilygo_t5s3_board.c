#include "lilygo_t5s3_board.h"
#include "epdiy.h"
#include "output_i2s/i2s_data_bus.h"
#include "output_i2s/rmt_pulse.h"

#include <driver/gpio.h>
#include <esp_log.h>

#define TAG "lilygo_s3"

typedef struct {
    bool ep_latch_enable : 1;
    bool power_disable : 1;
    bool pos_power_enable : 1;
    bool neg_power_enable : 1;
    bool ep_stv : 1;
    bool ep_scan_direction : 1;
    bool ep_mode : 1;
    bool ep_output_enable : 1;
} epd_config_register_t;

static epd_config_register_t config_reg;

#include <soc/gpio_struct.h>

inline static void fast_gpio_set_hi(gpio_num_t gpio_num) {
    if (gpio_num < 32) {
        GPIO.out_w1ts = (1 << gpio_num);
    } else {
        GPIO.out1_w1ts.val = (1 << (gpio_num - 32));
    }
}

inline static void fast_gpio_set_lo(gpio_num_t gpio_num) {
    if (gpio_num < 32) {
        GPIO.out_w1tc = (1 << gpio_num);
    } else {
        GPIO.out1_w1tc.val = (1 << (gpio_num - 32));
    }
}

static inline void IRAM_ATTR push_cfg_bit(bool bit) {
    fast_gpio_set_lo(LILYGO_T5S3_CFG_CLK);
    if (bit) {
        fast_gpio_set_hi(LILYGO_T5S3_CFG_DATA);
    } else {
        fast_gpio_set_lo(LILYGO_T5S3_CFG_DATA);
    }
    fast_gpio_set_hi(LILYGO_T5S3_CFG_CLK);
}

static void IRAM_ATTR push_cfg(epd_config_register_t *cfg) {
    fast_gpio_set_lo(LILYGO_T5S3_CFG_STR);
    // Push config bits in reverse order (74HC4094 serial in)
    push_cfg_bit(cfg->ep_output_enable);
    push_cfg_bit(cfg->ep_mode);
    push_cfg_bit(cfg->ep_scan_direction);
    push_cfg_bit(cfg->ep_stv);
    push_cfg_bit(cfg->neg_power_enable);
    push_cfg_bit(cfg->pos_power_enable);
    push_cfg_bit(cfg->power_disable);
    push_cfg_bit(cfg->ep_latch_enable);
    fast_gpio_set_hi(LILYGO_T5S3_CFG_STR);
}

static void lilygo_t5s3_init(uint32_t epd_row_width, const EpdInitConfig* init_config) {
    (void)init_config;
    ESP_LOGI(TAG, "Initializing LilyGo T5-S3 board driver...");

    gpio_set_direction(LILYGO_T5S3_CFG_DATA, GPIO_MODE_OUTPUT);
    gpio_set_direction(LILYGO_T5S3_CFG_CLK, GPIO_MODE_OUTPUT);
    gpio_set_direction(LILYGO_T5S3_CFG_STR, GPIO_MODE_OUTPUT);
    gpio_set_level(LILYGO_T5S3_CFG_STR, 0);

    config_reg.ep_latch_enable = false;
    config_reg.power_disable = true;
    config_reg.pos_power_enable = false;
    config_reg.neg_power_enable = false;
    config_reg.ep_stv = true;
    config_reg.ep_scan_direction = true;
    config_reg.ep_mode = false;
    config_reg.ep_output_enable = false;
    push_cfg(&config_reg);

    i2s_bus_config bus_cfg = {
        .data_0 = LILYGO_T5S3_D0,
        .data_1 = LILYGO_T5S3_D1,
        .data_2 = LILYGO_T5S3_D2,
        .data_3 = LILYGO_T5S3_D3,
        .data_4 = LILYGO_T5S3_D4,
        .data_5 = LILYGO_T5S3_D5,
        .data_6 = LILYGO_T5S3_D6,
        .data_7 = LILYGO_T5S3_D7,
        .clock = LILYGO_T5S3_CKH,
        .start_pulse = LILYGO_T5S3_STH,
    };

    i2s_bus_init(&bus_cfg, epd_row_width);
    rmt_pulse_init(LILYGO_T5S3_CKV);

    ESP_LOGI(TAG, "LilyGo T5-S3 board initialized successfully.");
}

static void lilygo_t5s3_deinit(void) {
    i2s_bus_deinit();
    rmt_pulse_deinit();
}

static void lilygo_t5s3_set_ctrl(epd_ctrl_state_t* state, const epd_ctrl_state_t* const mask) {
    if (mask->ep_output_enable || mask->ep_mode || mask->ep_stv || mask->ep_latch_enable) {
        if (mask->ep_output_enable) config_reg.ep_output_enable = state->ep_output_enable;
        if (mask->ep_mode) config_reg.ep_mode = state->ep_mode;
        if (mask->ep_stv) config_reg.ep_stv = state->ep_stv;
        if (mask->ep_latch_enable) config_reg.ep_latch_enable = state->ep_latch_enable;
        push_cfg(&config_reg);
    }
}

static void lilygo_t5s3_poweron(epd_ctrl_state_t* state) {
    (void)state;
    config_reg.ep_scan_direction = true;
    config_reg.power_disable = false;
    push_cfg(&config_reg);
    epd_busy_delay(100 * 240);

    config_reg.neg_power_enable = true;
    push_cfg(&config_reg);
    epd_busy_delay(500 * 240);

    config_reg.pos_power_enable = true;
    push_cfg(&config_reg);
    epd_busy_delay(100 * 240);

    config_reg.ep_stv = true;
    push_cfg(&config_reg);

    fast_gpio_set_hi(LILYGO_T5S3_STH);
}

static void lilygo_t5s3_poweroff(epd_ctrl_state_t* state) {
    (void)state;
    config_reg.pos_power_enable = false;
    push_cfg(&config_reg);
    epd_busy_delay(10 * 240);

    config_reg.neg_power_enable = false;
    push_cfg(&config_reg);
    epd_busy_delay(100 * 240);

    config_reg.power_disable = true;
    config_reg.ep_output_enable = false;
    config_reg.ep_mode = false;
    config_reg.ep_stv = false;
    push_cfg(&config_reg);
}

static void lilygo_t5s3_measure_vcom(epd_ctrl_state_t* state) {
    (void)state;
}

static float lilygo_t5s3_get_temperature(void) {
    return 22.0f;
}

static void lilygo_t5s3_set_vcom(int value) {
    (void)value;
}

const EpdBoardDefinition lilygo_t5s3_board = {
    .init = lilygo_t5s3_init,
    .deinit = lilygo_t5s3_deinit,
    .set_ctrl = lilygo_t5s3_set_ctrl,
    .poweron = lilygo_t5s3_poweron,
    .poweroff = lilygo_t5s3_poweroff,
    .measure_vcom = lilygo_t5s3_measure_vcom,
    .get_temperature = lilygo_t5s3_get_temperature,
    .set_vcom = lilygo_t5s3_set_vcom,
    .gpio_set_direction = NULL,
    .gpio_read = NULL,
    .gpio_write = NULL,
};
