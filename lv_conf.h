/**
 * lv_conf.h
 * Tailored for LILYGO T5 4.7" ePaper (960x540, 16 grayscale levels)
 * LVGL 8.3.x, used with EEZ Studio generated UI + LILYGO epd_driver
 */

#if 1 /* Set to 1 to enable content */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   COLOR SETTINGS
 *====================*/

#define LV_COLOR_DEPTH 16

/*====================
   DISPLAY SETTINGS
 *====================*/

#define LV_HOR_RES_MAX   960
#define LV_VER_RES_MAX   540

/*=========================
   MEMORY SETTINGS
 *=========================*/

#define LV_MEM_CUSTOM 0
#if LV_MEM_CUSTOM == 0
    #define LV_MEM_SIZE (64U * 1024U)          /* [bytes] - bump if you add many widgets/fonts */
    #define LV_MEM_ADR 0
#endif

/*====================
   HAL SETTINGS
 *====================*/

#define LV_DISP_DEF_REFR_PERIOD 30      /* ePaper is slow, no need to refresh fast */
#define LV_INDEV_DEF_READ_PERIOD 30

#define LV_TICK_CUSTOM 1
#if LV_TICK_CUSTOM
    #define LV_TICK_CUSTOM_INCLUDE "Arduino.h"
    #define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())
#endif

#define LV_DPI_DEF 130

/*=======================
 * FEATURE CONFIGURATION
 *======================*/

#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR 0

#define LV_USE_REFR_DEBUG 0

#define LV_USE_LOG 0
#if LV_USE_LOG
    #define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
    #define LV_LOG_PRINTF 1
#endif

#define LV_USE_ASSERT_NULL 1
#define LV_USE_ASSERT_MALLOC 1
#define LV_USE_ASSERT_STYLE 0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ 0

/*-------------
 * Widgets needed by your EEZ export: bar, btn, canvas, checkbox, img,
 * label, line, list
 *-----------*/
#define LV_USE_ARC        1
#define LV_USE_BAR        1
#define LV_USE_BTN        1
#define LV_USE_BTNMATRIX   1
#define LV_USE_CANVAS     1
#define LV_USE_CHECKBOX   1
#define LV_USE_DROPDOWN   1
#define LV_USE_IMG        1
#define LV_USE_LABEL      1
#define LV_USE_LINE       1
#define LV_USE_LIST       1
#define LV_USE_ROLLER     1
#define LV_USE_SLIDER     1
#define LV_USE_SWITCH     1
#define LV_USE_TABLE      1
#define LV_USE_TEXTAREA   1

#define LV_USE_THEME_DEFAULT 1
#if LV_USE_THEME_DEFAULT
    #define LV_THEME_DEFAULT_DARK 0
    #define LV_THEME_DEFAULT_GROW 1
    #define LV_THEME_DEFAULT_TRANSITION_TIME 0   /* No transitions, ePaper can't animate well */
#endif

#define LV_USE_FLEX 1
#define LV_USE_GRID 1

/*==================
 * FONT USAGE
 *==================*/
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_24 1
/* Custom fonts inter_medium_24pt and inter_bold_200pt come from the
 * EEZ export (ui_font_inter_medium_24pt.c / ui_font_inter_bold_200pt.c)
 * and are compiled directly, no lv_conf change needed for those. */

#define LV_TXT_ENC LV_TXT_ENC_UTF8

/*==================
 * OTHERS
 *==================*/
#define LV_USE_SNAPSHOT 0
#define LV_USE_GPU 0

#endif /*LV_CONF_H*/

#endif /*Content enable*/
