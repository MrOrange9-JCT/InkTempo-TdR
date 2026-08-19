#include <Arduino.h>
#include <Wire.h>
#include <TouchDrvGT911.hpp>

// EPDiy library headers
#include "epdiy.h"
#include "firasans_12.h"
#include "firasans_20.h"

// Standalone LilyGo T5-S3 board definition
#include "lilygo_t5s3_board.h"

#define TAG "main"

// High-level display state machine (dual framebuffers + diff engine)
static EpdiyHighlevelState hl;
static uint8_t* fb = nullptr;

// Touch controller instance
static TouchDrvGT911 touch;
static bool touch_available = false;

// Application state
static int current_page = 0;
static const int TOTAL_PAGES = 3;
static uint32_t flip_count = 0;
static uint32_t last_refresh_ms = 0;

// Read battery voltage (divider ~ 2:1)
static float read_battery_voltage() {
    uint32_t raw = analogRead(LILYGO_T5S3_BATT_ADC);
    // ADC reference ~ 3.3V, 12-bit (4095), 2x resistor divider
    float voltage = (raw / 4095.0f) * 3.3f * 2.0f * 1.05f;
    return voltage;
}

// Draw top status bar
static void draw_status_bar(const char* title, int page_num, int total_pages) {
    // Top bar background
    epd_fill_rect((EpdRect){0, 0, 960, 42}, 0xE0, fb);
    epd_draw_hline(0, 42, 960, 0x00, fb);

    // Title on left
    int x = 20;
    int y = 28;
    EpdFontProperties font_props = epd_font_properties_default();
    font_props.fg_color = 0x0;
    font_props.bg_color = 0xE;
    font_props.flags = EPD_DRAW_BACKGROUND;
    epd_write_string(&FiraSans_12, title, &x, &y, fb, &font_props);

    // Battery & Page on right
    float vbat = read_battery_voltage();
    char right_info[64];
    snprintf(right_info, sizeof(right_info), "Bat: %.2fV  |  Page %d/%d", vbat, page_num, total_pages);
    int rx = 720;
    int ry = 28;
    epd_write_string(&FiraSans_12, right_info, &rx, &ry, fb, &font_props);
}

// Draw bottom tip bar
static void draw_footer(const char* tip) {
    epd_fill_rect((EpdRect){0, 500, 960, 40}, 0xF0, fb);
    epd_draw_hline(0, 500, 960, 0x80, fb);

    int x = 480;
    int y = 526;
    EpdFontProperties font_props = epd_font_properties_default();
    font_props.fg_color = 0x2;
    font_props.flags = EPD_DRAW_ALIGN_CENTER;
    epd_write_string(&FiraSans_12, tip, &x, &y, fb, &font_props);
}

// Page 1: 16-Level Grayscale Palette & E-Reader Demo
static void draw_page_1() {
    epd_fill_rect((EpdRect){0, 0, 960, 540}, 0xF0, fb);
    draw_status_bar("EPDiy T5-S3 Demo  [ 16-Grayscale Palette ]", 1, TOTAL_PAGES);

    // Subtitle
    int x = 40;
    int y = 78;
    EpdFontProperties fp_sub = epd_font_properties_default();
    fp_sub.fg_color = 0x0;
    epd_write_string(&FiraSans_12, "16-Level True Grayscale Steps (0x00 Black to 0xF0 White):", &x, &y, fb, &fp_sub);

    // Render 16 Grayscale step boxes
    int box_w = 52;
    int box_h = 70;
    int start_x = 40;
    int start_y = 95;

    for (int i = 0; i < 16; i++) {
        uint8_t gray = (i << 4); // 0x00, 0x10, 0x20 ... 0xF0
        EpdRect r = { start_x + i * (box_w + 3), start_y, box_w, box_h };
        epd_fill_rect(r, gray, fb);
        epd_draw_rect(r, 0x00, fb); // Outline

        // Label step number
        char lbl[8];
        snprintf(lbl, sizeof(lbl), "%d", i);
        int lx = r.x + box_w / 2;
        int ly = r.y + box_h + 22;
        EpdFontProperties fp_lbl = epd_font_properties_default();
        fp_lbl.fg_color = 0x0;
        fp_lbl.flags = EPD_DRAW_ALIGN_CENTER;
        epd_write_string(&FiraSans_12, lbl, &lx, &ly, fb, &fp_lbl);
    }

    // Article Section
    int ax = 40;
    int ay = 230;
    epd_write_string(&FiraSans_12, "Why EPDiy Fast Refresh (MODE_GL16) is revolutionary for T5-S3:", &ax, &ay, fb, &fp_sub);

    const char* p1 = "1. Gray-to-Gray Direct Transitions: Pixels transition directly from current color to target color.";
    const char* p2 = "2. No Full Screen Inversion: Enjoy reading without distracting black-and-white flashing.";
    const char* p3 = "3. Sub-second Page Turns: ESP32-S3 LCD DMA bus streams high-speed parallel waveforms effortlessly.";
    const char* p4 = "4. Differential Framebuffer: Only dirty regions are calculated and pushed over the 8-bit bus.";

    int tx = 50;
    int ty = 270;
    EpdFontProperties fp_body = epd_font_properties_default();
    fp_body.fg_color = 0x2;
    epd_write_string(&FiraSans_12, p1, &tx, &ty, fb, &fp_body);
    ty += 34;
    epd_write_string(&FiraSans_12, p2, &tx, &ty, fb, &fp_body);
    ty += 34;
    epd_write_string(&FiraSans_12, p3, &tx, &ty, fb, &fp_body);
    ty += 34;
    epd_write_string(&FiraSans_12, p4, &tx, &ty, fb, &fp_body);

    draw_footer(">>> Tap Screen or Click Button 21 to turn page (Non-flashing GL16) <<<");
}

// Page 2: Grayscale Geometry & Shading Test
static void draw_page_2() {
    epd_fill_rect((EpdRect){0, 0, 960, 540}, 0xF0, fb);
    draw_status_bar("EPDiy T5-S3 Demo  [ Shading & Geometry ]", 2, TOTAL_PAGES);

    // Left side: Concentric shaded circles
    int cx = 220;
    int cy = 270;
    for (int r = 160; r > 10; r -= 15) {
        uint8_t color = ((160 - r) / 10) << 4;
        epd_fill_circle(cx, cy, r, color, fb);
        epd_draw_circle(cx, cy, r, 0x00, fb);
    }

    // Right side: 3D Shaded Buttons / Cards
    int rx = 480;
    int ry = 80;
    int rw = 430;
    int rh = 95;

    for (int i = 0; i < 4; i++) {
        EpdRect card = { rx, ry + i * (rh + 16), rw, rh };
        // Card shadow
        epd_fill_rect((EpdRect){ card.x + 6, card.y + 6, card.width, card.height }, 0x30, fb);
        // Card body
        uint8_t card_bg = 0xD0 + (i * 0x08);
        epd_fill_rect(card, card_bg, fb);
        epd_draw_rect(card, 0x00, fb);

        // Text inside card
        char ctxt[64];
        snprintf(ctxt, sizeof(ctxt), "Card #%d: Background Gray 0x%02X", i + 1, card_bg);
        int tx = card.x + 20;
        int ty = card.y + 36;
        EpdFontProperties fp = epd_font_properties_default();
        fp.fg_color = 0x0;
        epd_write_string(&FiraSans_12, ctxt, &tx, &ty, fb, &fp);

        snprintf(ctxt, sizeof(ctxt), "Diff engine only refreshes modified pixels smoothly.");
        tx = card.x + 20;
        ty = card.y + 68;
        fp.fg_color = 0x4;
        epd_write_string(&FiraSans_12, ctxt, &tx, &ty, fb, &fp);
    }

    draw_footer(">>> Tap Screen or Click Button 21 to turn page (Non-flashing GL16) <<<");
}

// Page 3: Live Stats & Refresh Comparison
static void draw_page_3() {
    epd_fill_rect((EpdRect){0, 0, 960, 540}, 0xF0, fb);
    draw_status_bar("EPDiy T5-S3 Demo  [ Performance & Comparison ]", 3, TOTAL_PAGES);

    // Big Stats Box
    EpdRect stats_box = { 40, 70, 880, 150 };
    epd_fill_rect(stats_box, 0xE8, fb);
    epd_draw_rect(stats_box, 0x00, fb);

    int x = 70;
    int y = 115;
    EpdFontProperties fp_big = epd_font_properties_default();
    fp_big.fg_color = 0x0;
    char stat_str[128];
    snprintf(stat_str, sizeof(stat_str), "Total Non-Flashing Page Turns: %lu", (unsigned long)flip_count);
    epd_write_string(&FiraSans_12, stat_str, &x, &y, fb, &fp_big);

    y += 40;
    snprintf(stat_str, sizeof(stat_str), "Last Refresh Time: %lu ms  |  Panel: ED047TC1 960x540  |  Mode: MODE_GL16", (unsigned long)last_refresh_ms);
    EpdFontProperties fp_info = epd_font_properties_default();
    fp_info.fg_color = 0x3;
    epd_write_string(&FiraSans_12, stat_str, &x, &y, fb, &fp_info);

    // Mode Comparison Table
    int ty = 250;
    int tx = 50;
    epd_write_string(&FiraSans_12, "Supported EPDiy Refresh Modes:", &tx, &ty, fb, &fp_big);

    int my = 295;
    const char* m1 = "• MODE_GL16 (Active): 16-level grayscale non-flashing fast transition. Best for page turns.";
    const char* m2 = "• MODE_GC16: Full clear 16-level grayscale refresh. Clears residual ghosting completely.";
    const char* m3 = "• MODE_DU: Fast 1-bit monochrome direct update (<150ms). Ideal for UI toggles & typing.";
    const char* m4 = "• MODE_EPDIY_WHITE_TO_GL16: Ultra-fast transition from white screen to 16 grayscale.";

    tx = 60;
    epd_write_string(&FiraSans_12, m1, &tx, &my, fb, &fp_info);
    my += 36;
    epd_write_string(&FiraSans_12, m2, &tx, &my, fb, &fp_info);
    my += 36;
    epd_write_string(&FiraSans_12, m3, &tx, &my, fb, &fp_info);
    my += 36;
    epd_write_string(&FiraSans_12, m4, &tx, &my, fb, &fp_info);

    draw_footer(">>> Tap to cycle back to Page 1  (Long press > 1.5s for Full GC16 Refresh) <<<");
}

// Render and refresh current page
static void refresh_display(bool full_refresh) {
    flip_count++;

    switch (current_page) {
        case 0: draw_page_1(); break;
        case 1: draw_page_2(); break;
        case 2: draw_page_3(); break;
        default: draw_page_1(); break;
    }

    uint32_t t0 = millis();
    epd_poweron();

    if (full_refresh) {
        Serial.printf("[EPDiy] Refreshing Page %d via MODE_GC16 (Full Clear Refresh)...\n", current_page + 1);
        epd_hl_update_screen(&hl, MODE_GC16, 25);
    } else {
        Serial.printf("[EPDiy] Refreshing Page %d via MODE_GL16 (Fast Non-Flashing Differential)...\n", current_page + 1);
        epd_hl_update_screen(&hl, MODE_GL16, 25);
    }

    epd_poweroff();
    last_refresh_ms = millis() - t0;
    Serial.printf("[EPDiy] Refresh completed in %lu ms!\n", (unsigned long)last_refresh_ms);
}

void setup() {
    Serial.begin(115200);
    delay(3000); // Wait for USB CDC to enumerate
    Serial.println("\n==========================================");
    Serial.println("  EPDiy for LilyGo T5-S3 16-Grayscale Demo");
    Serial.println("==========================================");

    // 1. Initialize Button 1
    pinMode(LILYGO_T5S3_BUTTON_1, INPUT_PULLUP);

    // 2. Initialize GT911 Touchscreen
    Wire.begin(LILYGO_T5S3_TOUCH_SDA, LILYGO_T5S3_TOUCH_SCL);
    touch.setPins(-1, LILYGO_T5S3_TOUCH_INT);
    if (touch.begin(Wire, 0x14, LILYGO_T5S3_TOUCH_SDA, LILYGO_T5S3_TOUCH_SCL) ||
        touch.begin(Wire, 0x5D, LILYGO_T5S3_TOUCH_SDA, LILYGO_T5S3_TOUCH_SCL)) {
        touch_available = true;
        Serial.println("[Touch] GT911 Touchscreen initialized successfully.");
    } else {
        Serial.println("[Touch] GT911 Touchscreen not detected (Button 21 & Serial active).");
    }

    // 3. Initialize EPDiy for LilyGo T5-S3 with ED047TC1 Display
    Serial.println("[EPDiy] Initializing EPDiy driver for LilyGo T5-S3...");
    epd_init(&lilygo_t5s3_board, &ED047TC1, EPD_LUT_64K);

    // 4. Initialize High-level Dual Framebuffer + Diff State
    Serial.println("[EPDiy] Allocating framebuffers in PSRAM...");
    hl = epd_hl_init(EPD_BUILTIN_WAVEFORM);
    hl.mirror_x = false; // Disable software horizontal mirroring (LilyGo hardware scan direction handles this)
    fb = epd_hl_get_framebuffer(&hl);

    // 5. Initial Full Screen Clear & Draw Page 1
    Serial.println("[EPDiy] Clearing screen to pristine white...");
    epd_poweron();
    epd_fullclear(&hl, 25);
    epd_poweroff();

    Serial.println("[EPDiy] Performing initial screen draw (Page 1)...");
    refresh_display(true);

    Serial.println("\n[Ready] Tap the screen or press Button 21 to turn pages!");
}

void loop() {
    static uint32_t btn_press_start = 0;
    static bool btn_was_down = false;
    static uint32_t last_touch_trigger = 0;

    // Check Touch Screen
    if (touch_available && millis() - last_touch_trigger > 500) {
        int16_t tx[5], ty[5];
        uint8_t touched = touch.getPoint(tx, ty, 1);
        if (touched > 0) {
            last_touch_trigger = millis();
            Serial.printf("[Touch] Tap detected at (%d, %d)! Flipping page...\n", tx[0], ty[0]);
            current_page = (current_page + 1) % TOTAL_PAGES;
            refresh_display(false);
        }
    }

    // Check Physical Button 21
    bool btn_down = (digitalRead(LILYGO_T5S3_BUTTON_1) == LOW);
    if (btn_down && !btn_was_down) {
        btn_press_start = millis();
        btn_was_down = true;
    } else if (!btn_down && btn_was_down) {
        uint32_t press_duration = millis() - btn_press_start;
        btn_was_down = false;

        if (press_duration > 1500) {
            // Long press (>1.5s): Full clear refresh (MODE_GC16)
            Serial.println("[Button] Long press detected! Performing Full GC16 Refresh...");
            refresh_display(true);
        } else if (press_duration > 50) {
            // Short press: Next page (MODE_GL16)
            Serial.println("[Button] Click detected! Flipping to next page (MODE_GL16)...");
            current_page = (current_page + 1) % TOTAL_PAGES;
            refresh_display(false);
        }
    }

    // Heartbeat every 3 seconds to show life in Serial Monitor
    static uint32_t last_heartbeat = 0;
    if (millis() - last_heartbeat > 3000) {
        last_heartbeat = millis();
        Serial.printf("[Heartbeat] Uptime: %lu s | Battery: %.2fV | Current Page: %d/%d | Touch: %s\n",
                      millis() / 1000, read_battery_voltage(), current_page + 1, TOTAL_PAGES,
                      touch_available ? "Ready" : "Disabled");
    }

    delay(20);
}
