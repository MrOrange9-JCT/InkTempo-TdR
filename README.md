# EPDiy for LilyGo T5 4.7" ESP32-S3 (16-Grayscale Smooth Fast Refresh)

---

## English Documentation

### 📌 Overview & Purpose

This project brings the powerful **EPDiy 16-level Grayscale Waveform Engine** and **`MODE_GL16` Smooth Non-Flashing Differential Fast Refresh** to the **LilyGo T5 4.7" ESP32-S3 E-Paper Display (ED047TC1 960×540)**.

While the stock firmware from LilyGo provides basic drawing capabilities, it lacks advanced gray-to-gray transition waveforms and true non-flashing differential updates, resulting in noticeable ghosting and frequent black-and-white full screen flashing during page turns.

By porting EPDiy's state-of-the-art rendering pipeline to this hardware, this project achieves:
- **16 True Grayscale Levels**: Crisp anti-aliased typography, gradient shading, and high-detail images.
- **Kindle-grade Non-Flashing Page Turns (`MODE_GL16`)**: Direct Gray-to-Gray transitions in ~750ms without distracting full-screen inversions.
- **Dual-Framebuffer Differential State Machine**: Only modified pixels receive electrical pulses in PSRAM, dramatically reducing ghosting.
- **Capacitive Touchscreen & Button Interaction**: Full support for GT911 touch tap page turns and IO 21 user button.

---

### 🔗 Reference Repositories & Hardware Documentation

- **Official EPDiy Core Project**: [https://github.com/vroland/epdiy](https://github.com/vroland/epdiy)
- **Official LilyGo T5 Series Wiki**: [https://wiki.lilygo.cc/zh/products/t5-series/t5-e-paper/](https://wiki.lilygo.cc/zh/products/t5-series/t5-e-paper/)
- **Official LilyGo-EPD47 Repository**: [https://github.com/Xinyuan-LilyGO/LilyGo-EPD47](https://github.com/Xinyuan-LilyGO/LilyGo-EPD47)

---

### ❓ Why Official EPDiy Does Not Support LilyGo T5-S3 Out-of-the-Box

If you try to run the official [vroland/epdiy](https://github.com/vroland/epdiy) repository directly on a LilyGo T5 4.7" ESP32-S3 board, the screen will remain completely blank or scramble pixels due to fundamental hardware and architectural mismatches:

1. **74HC4094 Shift Register vs. Direct GPIO Routing**:
   * **EPDiy Official Hardware (EPDiy v7)** connects all e-paper control lines (row latch `LEH`, gate frame start `STV`, high-voltage power rails `POS_EN`/`NEG_EN`) directly to ESP32-S3 GPIOs.
   * **LilyGo T5-S3** routes `LEH`, `STV`, and `POWER_EN` through a **74HC4094 serial shift register** (driven via `CFG_CLK`, `CFG_DATA`, and `CFG_STR`) to minimize GPIO count. Official EPDiy has no shift-register latching logic in its S3 driver.
2. **Hardcoded Continuous DMA on ESP32-S3**:
   * Official EPDiy's ESP32-S3 driver (`output_lcd/lcd_driver.c`) is hardcoded to use autonomous continuous DMA streaming across the entire display frame without per-row synchronization hooks.
   * LilyGo T5-S3 requires **row-by-row synchronized transmission** where a fast 74HC4094 strobe pulse must be sent after every single row of pixel data is pushed.
3. **Byte Ordering on ESP32-S3 Intel 8080 Bus**:
   * Legacy EPDiy performs 16-bit halfword swapping (`reorder_line_buffer`) to accommodate the old ESP32 I2S FIFO DMA quirks.
   * The ESP32-S3 Intel 8080 LCD peripheral transmits bytes sequentially in natural memory order. Running `reorder_line_buffer` on S3 transposes adjacent groups of pixels, corrupting text and shapes horizontally into double-vision duplicates.
4. **ESP-IDF 5.3+ I2C API Incompatibility with Arduino Core**:
   * Upstream EPDiy recently migrated to the new ESP-IDF 5.3+ I2C Master driver (`#include <driver/i2c_master.h>`) for its own v6/v7 boards, causing compilation errors on Arduino ESP32 2.0.x (which is based on ESP-IDF 4.4).

---

### 🛡️ Seamless Upstream Tracking via Automated Pre-Build Fixes

To solve these incompatibilities while allowing developers to **always stay up-to-date with upstream EPDiy releases**, this project uses an automated pre-build fix architecture:

1. **Direct Upstream Tracking**:
   [`platformio.ini`](platformio.ini) directly references the official upstream EPDiy repository:
   ```ini
   lib_deps =
       https://github.com/vroland/epdiy.git
   ```
2. **Automated External Hardware Patch (`patch_epdiy.py`)**:
   PlatformIO executes `extra_scripts = pre:patch_epdiy.py` before compilation. This script automatically injects the ESP32-S3 Intel 8080 row-level LCD driver, 74HC4094 shift register timing, RMT gate pulse clocking, and IDF 4.4 compatibility stubs into the pulled EPDiy code.
3. **Future-Proof**:
   Whenever upstream EPDiy releases new algorithms, waveforms, or performance enhancements, you can update without fear of losing hardware adaptations.

---

### 🔌 Hardware Pin Mapping

| Signal | Function | Hardware Connection |
| :--- | :--- | :--- |
| **D0 ~ D7** | 8-bit Parallel Data Bus | GPIO 8, 1, 2, 3, 4, 5, 6, 7 (Intel 8080 LCD Bus) |
| **CKH** | Source Driver Clock | GPIO 41 (Intel 8080 WR) |
| **STH** | Source Start Pulse / DC | GPIO 40 (Intel 8080 DC) |
| **CKV** | Gate Driver Clock | GPIO 38 (Hardware RMT Peripheral) |
| **CFG_CLK** | 74HC4094 Clock | GPIO 12 |
| **CFG_DATA** | 74HC4094 Serial Data | GPIO 13 |
| **CFG_STR** | 74HC4094 Strobe / Latch | GPIO 0 |
| **LEH** | Row Data Latch Enable | 74HC4094 Bit 0 |
| **STV** | Gate Frame Start Pulse | 74HC4094 Bit 4 |
| **POWER_EN** | E-Paper +/- High Voltage Power | 74HC4094 Bit 1, 2, 3 |
| **TOUCH_SDA/SCL**| GT911 Touchscreen I2C | GPIO 18 (SDA), GPIO 17 (SCL) |
| **TOUCH_INT** | GT911 Touch Interrupt | GPIO 47 |
| **BUTTON_1** | User Programmable Button | GPIO 21 |

---

### 🚀 Build & Flash Instructions

```bash
# 1. Build project (PlatformIO will automatically pull upstream EPDiy and apply patch)
pio run

# 2. Upload firmware to LilyGo T5-S3
pio run -t upload

# 3. Open Serial Monitor (115200 baud)
pio device monitor
```

---
---

## 中文说明文档

### 📌 项目概述与核心作用

本项目为 **LilyGo T5 4.7寸 ESP32-S3 电子墨水屏开发板 (ED047TC1 960×540)** 完整移植了 **EPDiy 官方 16 级灰阶波形算法引擎** 与 **`MODE_GL16` 无闪烁差分极速翻页驱动**。

LilyGo 原厂驱动虽然提供了基础的绘图功能，但缺乏完善的灰阶直转（Gray-to-Gray）波形和前后帧差分比对机制，翻页时容易产生明显的黑白全屏闪烁和大量残影。

通过本项目引入 EPDiy 的高阶渲染算法，实现了：
- **16 级真灰阶渲染**：呈现细腻的抗锯齿字体、平滑图形渐变和照片级图像。
- **电子书级无闪烁快速翻页 (`MODE_GL16`)**：像素由当前灰阶直接变换到目标灰阶，单次翻页仅需约 **750ms**，彻底告别刺眼的黑白颠倒闪烁。
- **双缓冲差分状态机（Dual Framebuffer）**：在 8MB PSRAM 中实时维护显存前后帧差异，只对发生变化的像素施加电脉冲，将残影抑制到极致。
- **触控与按键多模态交互**：支持 GT911 电容触摸屏轻触翻页及 IO 21 物理按键操作。

---

### 🔗 参考代码仓库与开发板官方资料

- **EPDiy 官方核心项目**：[https://github.com/vroland/epdiy](https://github.com/vroland/epdiy)
- **LilyGo 官方 T5 系列产品 Wiki**：[https://wiki.lilygo.cc/zh/products/t5-series/t5-e-paper/](https://wiki.lilygo.cc/zh/products/t5-series/t5-e-paper/)
- **LilyGo-EPD47 官方代码仓库**：[https://github.com/Xinyuan-LilyGO/LilyGo-EPD47](https://github.com/Xinyuan-LilyGO/LilyGo-EPD47)

---

### ❓ 为什么官方 EPDiy 原版无法直接驱动 LilyGo T5-S3？

如果您直接拉取官方 [vroland/epdiy](https://github.com/vroland/epdiy) 仓库并在 LilyGo T5 4.7" ESP32-S3 上运行，屏幕会处于**完全黑屏无反应**或**文字乱码重影**的状态，根本原因在于硬件架构与底层驱动的差异：

1. **74HC4094 移位寄存器控制 vs 直连 GPIO 方案**：
   * **EPDiy 官方硬件（如 EPDiy v7）** 将行数据锁存（`LEH`）、场同步脉冲（`STV`）和正负高压供电控制线全部**直连在 ESP32-S3 的物理 GPIO** 上。
   * **LilyGo T5-S3 硬件** 出于节省 GPIO 引脚的考虑，将 `LEH`、`STV` 和高压供电线串行挂接在了一颗 **74HC4094 移位寄存器芯片** 上（通过 `CFG_CLK`、`CFG_DATA`、`CFG_STR` 控制）。官方 EPDiy 库的 S3 驱动完全没有这套移位控制逻辑。
2. **ESP32-S3 驱动被硬编码为全自动连续 DMA**：
   * 官方 EPDiy 的 S3 渲染管道（`output_lcd/lcd_driver.c`）硬编码为一整帧连续触发 DMA，内部没有“每推完一行数据发送一次行锁存脉冲”的时序钩子。
   * LilyGo T5-S3 需要**逐行（Row-by-Row）受控发送**，每传输一行 8080 并行总线数据，必须紧接着给 74HC4094 发送高速锁存脉冲，屏端才能接收到数据。
3. **Intel 8080 总线与旧版 I2S 的字节顺序差异**：
   * 旧版 EPDiy 为了适应 ESP32 I2S FIFO 硬件特性，调用了 `reorder_line_buffer`（每两字节调换一次顺序）。
   * ESP32-S3 的 Intel 8080 LCD 并行外设是严格按内存顺序发出的。如果执行重排，会导致**相邻 8 个像素与后 8 个像素左右颠倒**，文字出现双重撕裂与乱码。
4. **ESP-IDF 5.3+ I2C 驱动更新导致 Arduino 编译报错**：
   * 官方 EPDiy 最新主干为了其自研板全面采用了 ESP-IDF 5.3+ 的 `i2c_master.h` 新驱动，这会导致在基于 IDF 4.4 的 Arduino ESP32 2.0.x 环境下编译直接报错中断。

---

### 🛡️ 外围自动 Patch 机制（无缝兼容 EPDiy 官方更新）

为了实现“**既能享受官方 EPDiy 最新代码库更新，又不需要手动反复修改第三方库**”的目标，本项目采用了解耦的外围自动补丁架构：

1. **直接引用官方最新 Git 主干**：
   在 [`platformio.ini`](platformio.ini) 中直接配置官方库：
   ```ini
   lib_deps =
       https://github.com/vroland/epdiy.git
   ```
2. **自动化预编译修补（`patch_epdiy.py`）**：
   每次执行编译或更新库时，PlatformIO 会通过 `extra_scripts = pre:patch_epdiy.py` 自动执行修补脚本，毫秒级将 ESP32-S3 Intel 8080 并行总线驱动、RMT 门极时钟脉冲引擎、74HC4094 逐行锁存管道和 IDF 4.4 兼容桩自动注入到拉取的官方代码中。
3. **长久维护优势**：
   项目主目录保持极简干净，无冗余第三方源码。上游 EPDiy 发布新算法或修复时，无需任何手动介入即可自动更新并无缝构建。

---

### 🔌 硬件引脚映射表

| 信号名称 | 作用说明 | 硬件连接与外设 |
| :--- | :--- | :--- |
| **D0 ~ D7** | 8位数据并行总线 | GPIO 8, 1, 2, 3, 4, 5, 6, 7 (Intel 8080 LCD 总线) |
| **CKH** | 数据采样时钟 | GPIO 41 (Intel 8080 WR) |
| **STH** | 行起始脉冲 / DC | GPIO 40 (Intel 8080 DC) |
| **CKV** | 门极移位时钟 | GPIO 38 (硬件 RMT 脉冲发生器) |
| **CFG_CLK** | 移位寄存器时钟 | GPIO 12 |
| **CFG_DATA** | 移位寄存器数据 | GPIO 13 |
| **CFG_STR** | 移位寄存器锁存使能 | GPIO 0 |
| **LEH** | 行数据锁存使能 | 74HC4094 移位寄存器 Bit 0 |
| **STV** | 场同步起始脉冲 | 74HC4094 移位寄存器 Bit 4 |
| **POWER_EN** | 墨水屏正负高压供电 | 74HC4094 移位寄存器 Bit 1, 2, 3 |
| **TOUCH_SDA/SCL**| GT911 触摸 I2C | GPIO 18 (SDA), GPIO 17 (SCL) |
| **TOUCH_INT** | 触摸中断引脚 | GPIO 47 |
| **BUTTON_1** | 用户可编程按键 | GPIO 21 |

---

### 🛠️ 编译与烧录

```bash
# 1. 编译固件（自动拉取官方最新 EPDiy 并自动应用外围补丁）
pio run

# 2. 烧录固件到开发板
pio run -t upload

# 3. 打开串口监视器
pio device monitor
```

---

### 📖 交互操作指南

- **点击屏幕右半区 / 短按按键 21**：下一页（使用 `MODE_GL16` 无闪烁快速差分刷新）。
- **点击屏幕左半区**：上一页。
- **长按按键 21（> 1.5 秒）**：触发 `MODE_GC16` 全屏全刷清除残影。
