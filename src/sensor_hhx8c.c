/********************************** (C) COPYRIGHT *******************************
 * Copyright (c) 2026 createskyblue@outlook.com MIT
 *******************************************************************************/
/**
 ******************************************************************************
 * @file    sensor_hhx8c.c
 * @brief   Huashenke HH-X8C-10000 NDIR CO2 driver implementation
 *
 * Vendor: Henan Huashenke Intelligent Technology Co., Ltd
 * Model:  HH-X8C-10000
 *
 * Query mode: send read command, then SEEK → RECV → VERIFY the reply.
 * Results go only to the caller's out buffer. This driver does not flush UART.
 ******************************************************************************
 */

#include "sensor_hhx8c.h"

/* Read-concentration command (datasheet 0x86) */
static const uint8_t hhx8c_cmd_read[HHX8C_FRAME_LEN] = {
    0xFFu, 0x01u, 0x86u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x79u
};

#define HHX8C_IDX_START   0u
#define HHX8C_IDX_CMD     1u
#define HHX8C_IDX_CONC_HI 2u
#define HHX8C_IDX_CONC_LO 3u
#define HHX8C_IDX_CHECK   8u

typedef enum {
    HHX8C_ST_SEEK = 0,
    HHX8C_ST_RECV,
    HHX8C_ST_VERIFY
} hhx8c_state_t;

void sensor_hhx8c_init(sensor_hhx8c_t *ctx, const sensor_io_t *io)
{
    ctx->io = io;
    ctx->warmup_start_ms = (io != NULL && io->now_ms != NULL) ? io->now_ms() : 0u;
}

uint32_t sensor_hhx8c_get(sensor_hhx8c_t *ctx, sensor_hhx8c_data_t *out, uint32_t wait_ms)
{
    uint8_t       frame[HHX8C_FRAME_LEN];
    uint8_t       idx = 0u;
    uint8_t       sum;
    uint8_t       i;
    hhx8c_state_t state = HHX8C_ST_SEEK;
    uint32_t      deadline;

    if (ctx == NULL || out == NULL || ctx->io == NULL ||
        ctx->io->read == NULL || ctx->io->write == NULL ||
        ctx->io->now_ms == NULL) {
        return HHX8C_ERR_IO;
    }

    deadline = ctx->io->now_ms() + wait_ms;

    /* Always query (even during warm-up); warm-up is checked after a good parse */
    if (ctx->io->write(hhx8c_cmd_read, HHX8C_FRAME_LEN) != HHX8C_FRAME_LEN) {
        return HHX8C_ERR_IO;
    }

    do {
        uint8_t byte;

        if (ctx->io->read(&byte, 1u) != 1u) {
            continue;
        }

        switch (state) {
        case HHX8C_ST_SEEK:
            if (byte != HHX8C_START_BYTE) {
                break;
            }
            frame[HHX8C_IDX_START] = byte;
            idx                    = 1u;
            state                  = HHX8C_ST_RECV;
            break;

        case HHX8C_ST_RECV:
            frame[idx++] = byte;
            if (idx < HHX8C_FRAME_LEN) {
                break;
            }
            state = HHX8C_ST_VERIFY;
            /* FALLTHRU */

        case HHX8C_ST_VERIFY:
            if (frame[HHX8C_IDX_CMD] != HHX8C_CMD_READ) {
                state = HHX8C_ST_SEEK;
                idx   = 0u;
                break;
            }
            sum = 0u;
            for (i = HHX8C_IDX_CMD; i <= (HHX8C_IDX_CHECK - 1u); i++) {
                sum = (uint8_t)(sum + frame[i]);
            }
            if ((uint8_t)((uint8_t)(~sum) + 1u) != frame[HHX8C_IDX_CHECK]) {
                state = HHX8C_ST_SEEK;
                idx   = 0u;
                break;
            }
            /* Reply accepted — warm-up here so it is not confused with I/O errors */
            if ((uint32_t)(ctx->io->now_ms() - ctx->warmup_start_ms) < HHX8C_WARMUP_MS) {
                return HHX8C_ERR_WARMUP;
            }
            out->co2_ppm = (float)(((uint16_t)frame[HHX8C_IDX_CONC_HI] << 8) |
                                    (uint16_t)frame[HHX8C_IDX_CONC_LO]);
            out->valid   = 1u;
            return HHX8C_OK;
        }
    } while ((int32_t)(ctx->io->now_ms() - deadline) < 0);

    return HHX8C_ERR_IO;
}
