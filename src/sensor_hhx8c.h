/********************************** (C) COPYRIGHT *******************************
 * Copyright (c) 2026 createskyblue@outlook.com MIT
 *******************************************************************************/
/**
 ******************************************************************************
 * @file    sensor_hhx8c.h
 * @brief   Platform-independent driver for Huashenke HH-X8C-10000 NDIR CO2
 *
 * Vendor:  Henan Huashenke Intelligent Technology Co., Ltd
 * Model:   HH-X8C-10000 (NDIR infrared CO2)
 * Link:    UART 9600/8N1, query protocol (host-initiated), 9-byte frames
 *
 * Each get() sends a read command then parses the reply. io->write is
 * required. Zero / SPAN calibration commands are not implemented.
 ******************************************************************************
 */

#ifndef SENSOR_HHX8C_H_
#define SENSOR_HHX8C_H_

#include "sensor.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Protocol constants */
#define HHX8C_FRAME_LEN     9u
#define HHX8C_START_BYTE    0xFFu
#define HHX8C_CMD_READ      0x86u
#define HHX8C_RANGE_MIN     400.0f
#define HHX8C_RANGE_MAX     10000.0f

#ifndef HHX8C_WAIT_MS_TYPICAL
#define HHX8C_WAIT_MS_TYPICAL 1500u
#endif

/*! Warm-up after init: 3 minutes (checked only after a successful parse) */
#ifndef HHX8C_WARMUP_MS
#define HHX8C_WARMUP_MS     180000u
#endif

/* get() results: parse first, then warm-up — avoids mixing with I/O errors */
#define HHX8C_OK            0u  /*!< Reply parsed OK and warm-up done; out written */
#define HHX8C_ERR_IO        1u  /*!< Bad args / write fail / no reply; out untouched */
#define HHX8C_ERR_WARMUP    2u  /*!< Reply OK but warm-up not finished; out untouched */

/*! Parsed sample */
typedef struct {
    uint8_t valid;          /*!< 1 = at least one good reply received */
    float   co2_ppm;        /*!< CO2, ppm, range 400–10000 */
} sensor_hhx8c_data_t;

/*!
 * Driver context. No internal sample cache. Warm-up is judged after
 * a successful parse.
 */
typedef struct {
    const sensor_io_t *io;
    uint32_t warmup_start_ms;
} sensor_hhx8c_t;

/**
 * @brief Inject IO and record warm-up start time
 */
void sensor_hhx8c_init(sensor_hhx8c_t *ctx, const sensor_io_t *io);

/**
 * @brief Query once and fill out on success
 * @retval HHX8C_OK (0) — reply parsed, warm-up finished
 * @retval HHX8C_ERR_IO (1) — bad args, write failed, or no reply
 * @retval HHX8C_ERR_WARMUP (2) — reply OK but still within 3-minute warm-up
 *
 * Warm-up is checked after VERIFY and before writing out so WARMUP
 * means the link is healthy. io->write is mandatory (query mode).
 * Shared-UART flush is the caller's job.
 */
uint32_t sensor_hhx8c_get(sensor_hhx8c_t *ctx, sensor_hhx8c_data_t *out, uint32_t wait_ms);

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_HHX8C_H_ */
