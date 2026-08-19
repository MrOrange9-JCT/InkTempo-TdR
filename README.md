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

### 🔗 Hardware & Official Documentation References

This project is built specifically for the LilyGo T5 4.7" ESP32-S3 development board. For official hardware schematics, pin definitions, and product documentation, refer to:
- **Official LilyGo T5 Series Wiki**: [https://wiki.lilygo.cc/zh/products/t5-series/t5-e-paper/](https://wiki.lilygo.cc/zh/products/t5-series/t5-e-paper/)
- **Official LilyGo-EPD47 Repository**: [https://github.com/Xinyuan-LilyGO/LilyGo-EPD47](https://github.com/Xinyuan-LilyGO/LilyGo-EPD47)

---

### 🛡️ Seamless Upstream Tracking via Automated Pre-Build Fixes

A key architectural highlight of this project is **Zero Code Forking / Zero Manual Maintenance**:

1. **Direct Upstream Tracking**:
   `platformio.ini` directly references the official upstream EPDiy repository:
   ```ini
   lib_deps =
       https://github.com/vroland/epdiy.git
   ```
2. **Automated External Hardware Patch (`patch_epdiy.py`)**:
   Upstream EPDiy was originally designed for EPDiy v7 boards where all pins directly connect to ESP32 GPIOs. In contrast, LilyGo T5-S3 routes the row latch (`LEH`), frame start (`STV`), and high-voltage power gates through a **74HC4094 serial shift register**.
3. **How It Works**:
   During every compilation or library update, PlatformIO executes `extra_scripts = pre:patch_epdiy.py`. This script automatically injects the ESP32-S3 Intel 8080 parallel LCD bus driver, RMT gate pulse clocking, and 74HC4094 shift register row-latching pipeline into the pulled EPDiy code.
4. **Future-Proof**:
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
# 1. Build project (PlatformIO will automatically pull EPDiy and apply patch)
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

### 🔗 开发板官方资料与参考仓库

本项目专为 LilyGo T5 4.7" ESP32-S3 开发板定制开发，官方硬件原理图、引脚定义和产品维基请参考：
- **LilyGo 官方 T5 系列产品 Wiki**：[https://wiki.lilygo.cc/zh/products/t5-series/t5-e-paper/](https://wiki.lilygo.cc/zh/products/t5-series/t5-e-paper/)
- **LilyGo-EPD47 官方代码仓库**：[https://github.com/Xinyuan-LilyGO/LilyGo-EPD47](https://github.com/Xinyuan-LilyGO/LilyGo-EPD47)

---

### 🛡️ 外围自动 Patch 机制（无缝兼容 EPDiy 官方更新）

为了实现“**既能享受官方 EPDiy 最新代码库更新，又不需要手动反复修改第三方库**”的目标，本项目采用了解耦的外围自动补丁架构：

1. **直接引用官方最新 Git 主干**：
   在 [`platformio.ini`](file:///F:/BaiduNetdiskDownload/SoftwareProject/eink-screen/EPDiy%20for%20LilyGoT5S3/platformio.ini) 中直接配置官方库：
   ```ini
   lib_deps =
       https://github.com/vroland/epdiy.git
   ```
2. **为什么需要外围自动 Fix 脚本 (`patch_epdiy.py`)**：
   官方 EPDiy 原版代码是针对其自研的 EPDiy v7 硬件编写的（所有控制引脚直连 ESP32 GPIO）。而 LilyGo T5-S3 硬件出于引脚复用设计，将行数据锁存（`LEH`）、场同步脉冲（`STV`）和正负高压供电集成在了一颗 **74HC4094 移位寄存器芯片** 上。
3. **自动化构建流程**：
   每次执行编译或更新库时，PlatformIO 会通过 `extra_scripts = pre:patch_epdiy.py` 自动执行修补脚本，毫秒级将 ESP32-S3 Intel 8080 并行总线、RMT 脉冲引擎和 74HC4094 逐行锁存管道注入到官方代码中。
4. **长久维护优势**：
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
