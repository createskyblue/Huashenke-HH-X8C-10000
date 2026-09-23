**语言 / Language:** [English](README.md) | [中文](README.zh-CN.md)

# 华深科 HH-X8C-10000 · 红外 CO2 传感器驱动

**厂商：** 河南华深科智能科技有限公司  
**型号：** HH-X8C-10000  
**类型：** NDIR 红外 CO₂，UART **问讯式**（query）  

面向华深科 **HH-X8C-10000** 红外二氧化碳传感器的平台无关 C 驱动。

![HH-X8C-10000 红外二氧化碳传感器](img/PixPin_2026-09-24_03-56-36.jpg)

*HH-X8C-10000 红外二氧化碳传感器 · 河南华深科智能科技有限公司*

---

## 特性

- 平台无关：`sensor_io_t` 注入 `read` / `write` / `now_ms`（问讯必须有 `write`）
- 问讯式：每次 `get()` 由驱动完成收发，调用方不用拼命令
- 量程 400–10000 ppm，`float co2_ppm`
- 预热 3 分钟：**解析成功后**才返回 `HHX8C_ERR_WARMUP`
- 返回值：`0` 成功 / `1` 参数或写失败或无应答 / `2` 预热中
- **只读浓度**，不发送零点 / SPAN 校准命令

## 需要实现的 IO 接口

驱动不碰 HAL，只依赖你在 `init` 时注入的三个函数指针（见 `src/sensor.h`）：

```c
typedef struct sensor_io {
    /* 读串口：非阻塞，返回本次真正读到的字节数；0 = 暂无数据 */
    uint32_t (*read)(uint8_t *buf, uint32_t len);

    /* 写串口：问讯式必填。非阻塞，返回本次真正写出的字节数；
     * 写不满 len 时驱动会判失败（或你在这里循环补写直到写完再返回 len） */
    uint32_t (*write)(const uint8_t *buf, uint32_t len);

    /* 毫秒时钟：必填；用于超时与预热计时 */
    uint32_t (*now_ms)(void);
} sensor_io_t;
```

约定：

1. **`read` / `write` 必须非阻塞**——内部禁止 `delay` / 等发送完成标志；读空返回 0，由驱动重试到 `wait_ms` 截止。
2. **`write` 本驱动必填**：每次 `get()` 会先下发读命令；写失败直接返回 `HHX8C_ERR_IO`。
3. **`now_ms` 返回开机毫秒数**（允许回绕），预热与总预算都基于它。
4. 多传感器共用 UART 时，**切通道后清 RX 缓冲是调用方的职责**（取数前 `flush`）。

驱动保持平台无关：`init` 注入 `io` 即可，不绑定具体 MCU 或 RTOS。

## 用法

```c
#include "sensor_hhx8c.h"

sensor_hhx8c_t ctx;
sensor_hhx8c_data_t data = {0};

sensor_hhx8c_init(&ctx, &io);           /* io.write 必填 */

uint32_t err = sensor_hhx8c_get(&ctx, &data, HHX8C_WAIT_MS_TYPICAL);
if (err == HHX8C_OK) {
    /* data.co2_ppm */
} else if (err == HHX8C_ERR_WARMUP) {
    /* 链路正常，仍在 3 分钟预热内 */
} else {
    /* 写失败 / 超时 / 校验失败 / 参数错误 */
}
```

编译：把 `src/*.c` 加入工程，`src/` 加入 include 路径。

## 文件结构

| 路径 | 说明 |
|------|------|
| `src/sensor.h` | 依赖注入（IO 接口） |
| `src/sensor_hhx8c.h/.c` | 本传感器驱动 |
| `img/` | 产品图 |
| `docs/HH-X8C-10000二氧化碳说明书.docx` | 厂家规格书 |

---

**License:** MIT  
**规格书:** 见 `docs/`
