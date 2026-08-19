#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "epd_board.h"
#include <driver/gpio.h>
#include <esp_err.h>

// LilyGo T5 4.7" ESP32-S3 Hardware Pin Definitions
#define LILYGO_T5S3_CFG_DATA    GPIO_NUM_13
#define LILYGO_T5S3_CFG_CLK     GPIO_NUM_12
#define LILYGO_T5S3_CFG_STR     GPIO_NUM_0

#define LILYGO_T5S3_CKV         GPIO_NUM_38
#define LILYGO_T5S3_STH         GPIO_NUM_40
#define LILYGO_T5S3_CKH         GPIO_NUM_41

#define LILYGO_T5S3_D0          GPIO_NUM_8
#define LILYGO_T5S3_D1          GPIO_NUM_1
#define LILYGO_T5S3_D2          GPIO_NUM_2
#define LILYGO_T5S3_D3          GPIO_NUM_3
#define LILYGO_T5S3_D4          GPIO_NUM_4
#define LILYGO_T5S3_D5          GPIO_NUM_5
#define LILYGO_T5S3_D6          GPIO_NUM_6
#define LILYGO_T5S3_D7          GPIO_NUM_7

#define LILYGO_T5S3_TOUCH_SDA   GPIO_NUM_18
#define LILYGO_T5S3_TOUCH_SCL   GPIO_NUM_17
#define LILYGO_T5S3_TOUCH_INT   GPIO_NUM_47
#define LILYGO_T5S3_BUTTON_1    GPIO_NUM_21
#define LILYGO_T5S3_BATT_ADC    GPIO_NUM_14

/**
 * EpdBoardDefinition instance for LilyGo T5 4.7" ESP32-S3.
 * Pass this to epd_init(&lilygo_t5s3_board, &ED047TC1, EPD_LUT_64K).
 */
extern const EpdBoardDefinition lilygo_t5s3_board;

#ifdef __cplusplus
}
#endif
