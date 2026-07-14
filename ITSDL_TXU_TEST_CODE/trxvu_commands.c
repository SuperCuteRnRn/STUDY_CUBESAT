/* ============================================================================
 * trxvu_commands.c — 31 명령 wrapper 구현
 * (PLAN_TRX_HW_TEST_STANDALONE_20260521.md, 2026-05-21)
 *
 * Wire 인코딩:
 *   - SetFrequency (0x32):  4-byte payload (Hz 단위, ICD-0001 §7 endian 확인 필요)
 *   - GetFrequency (0x33):  4-byte response
 *   - SetTrpFreq (0x80):    4-byte LE (kHz, ICD-0002 §5.1.1.4)
 *   - GetTrpFreq (0x81):    4-byte LE freq + 1-byte status
 *   - Set RSSI threshold (0x52): 2-byte BE (ICD-0002 §5.1.1.3)
 *   - Get RSSI (0x51):       2-byte response (12-bit packed)
 * ========================================================================== */
#include "trxvu_commands.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/* settle delay 표준 (보드 응답 시간) */
#define TRXVU_CMD_SETTLE_MS  5U

/* ============================================================
 * Reset (위험 — CLI 측 confirm prompt 필수)
 * ============================================================ */

int trxvu_cmd_watchdog_reset(trxvu_ctx_t *ctx, uint8_t slave)
{
    return trxvu_write(ctx, slave, TRXVU_CC_WATCHDOG_RESET, NULL, 0U);
}

int trxvu_cmd_software_reset(trxvu_ctx_t *ctx, uint8_t slave)
{
    return trxvu_write(ctx, slave, TRXVU_CC_SOFTWARE_RESET, NULL, 0U);
}

int trxvu_cmd_hardware_reset(trxvu_ctx_t *ctx, uint8_t slave)
{
    return trxvu_write(ctx, slave, TRXVU_CC_HARDWARE_RESET, NULL, 0U);
}

/* ============================================================
 * Common
 * ============================================================ */

int trxvu_cmd_get_uptime(trxvu_ctx_t *ctx, uint8_t slave, uint32_t *out_seconds)
{
    int     result = -1;
    uint8_t rx[4]  = {0U};
    int     n;

    if (out_seconds == NULL)
    {
        errno = EINVAL;
    }
    else
    {
        n = trxvu_write_read(ctx, slave, TRXVU_CC_GET_UPTIME, NULL, 0U, rx, sizeof(rx));
        if (n == (int)sizeof(rx))
        {
            /* ICD-0001 §7: little-endian uptime in seconds */
            *out_seconds = (uint32_t)rx[0]
                         | ((uint32_t)rx[1] << 8)
                         | ((uint32_t)rx[2] << 16)
                         | ((uint32_t)rx[3] << 24);
            result = 0;
        }
    }
    return result;
}

int trxvu_cmd_set_frequency(trxvu_ctx_t *ctx, uint8_t slave, uint32_t freq_khz)
{
    uint8_t data[4];

    /* ICD-0001 §7: little-endian (cFS i2c_lib trx_functions.c:231 동일 패턴) */
    data[0] = (uint8_t)(freq_khz & 0xFFU);
    data[1] = (uint8_t)((freq_khz >> 8)  & 0xFFU);
    data[2] = (uint8_t)((freq_khz >> 16) & 0xFFU);
    data[3] = (uint8_t)((freq_khz >> 24) & 0xFFU);

    return trxvu_write(ctx, slave, TRXVU_CC_SET_FREQUENCY, data, sizeof(data));
}

int trxvu_cmd_get_frequency(trxvu_ctx_t *ctx, uint8_t slave, uint32_t *out_khz)
{
    int     result = -1;
    uint8_t rx[4]  = {0U};
    int     n;

    if (out_khz == NULL)
    {
        errno = EINVAL;
    }
    else
    {
        n = trxvu_write_read(ctx, slave, TRXVU_CC_GET_FREQUENCY, NULL, 0U, rx, sizeof(rx));
        if (n == (int)sizeof(rx))
        {
            *out_khz = (uint32_t)rx[0]
                     | ((uint32_t)rx[1] << 8)
                     | ((uint32_t)rx[2] << 16)
                     | ((uint32_t)rx[3] << 24);
            result = 0;
        }
    }
    return result;
}

int trxvu_cmd_get_pll_err(trxvu_ctx_t *ctx, uint8_t slave,
                          uint16_t *out_lock_err, uint16_t *out_freq_err)
{
    int     result = -1;
    uint8_t rx[4]  = {0U};
    int     n;

    if ((out_lock_err == NULL) || (out_freq_err == NULL))
    {
        errno = EINVAL;
    }
    else
    {
        n = trxvu_write_read(ctx, slave, TRXVU_CC_GET_PLL_ERR, NULL, 0U, rx, sizeof(rx));
        if (n == (int)sizeof(rx))
        {
            *out_lock_err = (uint16_t)rx[0] | ((uint16_t)rx[1] << 8);
            *out_freq_err = (uint16_t)rx[2] | ((uint16_t)rx[3] << 8);
            result = 0;
        }
    }
    return result;
}

int trxvu_cmd_get_firmware_info(trxvu_ctx_t *ctx, uint8_t slave, char out_buf[80])
{
    int result = -1;
    int n;

    if (out_buf == NULL)
    {
        errno = EINVAL;
    }
    else
    {
        n = trxvu_write_read(ctx, slave, TRXVU_CC_GET_FIRMWARE_INFO, NULL, 0U,
                             (uint8_t *)out_buf, 80U);
        if (n > 0)
        {
            /* null-terminate at last byte for safety */
            out_buf[79] = '\0';
            result = 0;
        }
    }
    return result;
}

int trxvu_cmd_get_last_reset_cause(trxvu_ctx_t *ctx, uint8_t slave, uint8_t *out_cause)
{
    int     result = -1;
    uint8_t rx[1]  = {0U};
    int     n;

    if (out_cause == NULL)
    {
        errno = EINVAL;
    }
    else
    {
        /* ICD-0001 §7.2.1.13 / §7.3.1.21: response = 1 byte */
        n = trxvu_write_read(ctx, slave, TRXVU_CC_GET_LAST_RESET, NULL, 0U, rx, sizeof(rx));
        if (n == (int)sizeof(rx))
        {
            *out_cause = rx[0];
            result = 0;
        }
    }
    return result;
}

/* ============================================================
 * RX (slave 0x60)
 * ============================================================ */

int trxvu_cmd_rcv_telemetry(trxvu_ctx_t *ctx, uint8_t out_buf[16])
{
    int n;
    int result = -1;

    if (out_buf == NULL)
    {
        errno = EINVAL;
    }
    else
    {
        n = trxvu_write_read(ctx, TRXVU_RCV_ADDR, TRXVU_CC_RCV_TELEMETRY, NULL, 0U, out_buf, 16U);
        if (n > 0)
        {
            result = n;
        }
    }
    return result;
}

int trxvu_cmd_rcv_get_frame_num(trxvu_ctx_t *ctx, uint16_t *out_num)
{
    int     result = -1;
    uint8_t rx[2]  = {0U};
    int     n;

    if (out_num == NULL)
    {
        errno = EINVAL;
    }
    else
    {
        n = trxvu_write_read(ctx, TRXVU_RCV_ADDR, TRXVU_CC_RCV_GET_FRAME_NUM,
                             NULL, 0U, rx, sizeof(rx));
        if (n == (int)sizeof(rx))
        {
            *out_num = (uint16_t)rx[0] | ((uint16_t)rx[1] << 8);
            result = 0;
        }
    }
    return result;
}

int trxvu_cmd_rcv_get_length(trxvu_ctx_t *ctx, uint16_t *out_len)
{
    int     result = -1;
    uint8_t rx[2]  = {0U};
    int     n;

    if (out_len == NULL)
    {
        errno = EINVAL;
    }
    else
    {
        n = trxvu_write_read(ctx, TRXVU_RCV_ADDR, TRXVU_CC_RCV_GET_LENGTH,
                             NULL, 0U, rx, sizeof(rx));
        if (n == (int)sizeof(rx))
        {
            *out_len = (uint16_t)rx[0] | ((uint16_t)rx[1] << 8);
            result = 0;
        }
    }
    return result;
}

int trxvu_cmd_rcv_get_frame(trxvu_ctx_t *ctx, uint8_t *out_frame, size_t max_len,
                            size_t *out_actual)
{
    int result = -1;
    int n;

    if ((out_frame == NULL) || (out_actual == NULL) || (max_len < 5U))
    {
        errno = EINVAL;
    }
    else
    {
        n = trxvu_write_read(ctx, TRXVU_RCV_ADDR, TRXVU_CC_RCV_GET_FRAME,
                             NULL, 0U, out_frame, max_len);
        if (n > 0)
        {
            *out_actual = (size_t)n;
            result = 0;
        }
    }
    return result;
}

int trxvu_cmd_rcv_remove_frame(trxvu_ctx_t *ctx)
{
    return trxvu_write(ctx, TRXVU_RCV_ADDR, TRXVU_CC_RCV_REMOVE_FRAME, NULL, 0U);
}

int trxvu_cmd_rcv_remove_all(trxvu_ctx_t *ctx)
{
    return trxvu_write(ctx, TRXVU_RCV_ADDR, TRXVU_CC_RCV_REMOVE_ALL, NULL, 0U);
}

/* ============================================================
 * TX (slave 0x61)
 * ============================================================ */

int trxvu_cmd_trs_send_frame(trxvu_ctx_t *ctx, const uint8_t *data, uint8_t len,
                             uint8_t *out_remaining)
{
    int     result = -1;
    uint8_t rx[1]  = {0U};
    int     n;

    if ((data == NULL) || (len == 0U) || (out_remaining == NULL))
    {
        errno = EINVAL;
    }
    else
    {
        n = trxvu_write_read(ctx, TRXVU_TRS_ADDR, TRXVU_CC_TRS_SEND_FRAME,
                             data, len, rx, sizeof(rx));
        if (n == (int)sizeof(rx))
        {
            *out_remaining = rx[0];
            result = 0;
        }
    }
    return result;
}

int trxvu_cmd_trs_set_beacon(trxvu_ctx_t *ctx, uint16_t interval_s,
                             const uint8_t *data, uint8_t len)
{
    int     result = -1;
    uint8_t buf[2U + 255U];

    if ((data == NULL) || (len == 0U) || (len > 235U))
    {
        errno = EINVAL;
    }
    else
    {
        /* ICD-0001 §7.3.1.3: [001-002] xxxxxxxx 0000xxxx
         * byte[1] upper nibble must be 0 */
        buf[0] = (uint8_t)(interval_s & 0xFFU);
        buf[1] = (uint8_t)((interval_s >> 8) & 0x0FU);
        (void)memcpy(&buf[2], data, len);
        result = trxvu_write(ctx, TRXVU_TRS_ADDR, TRXVU_CC_TRS_SET_BEACON,
                             buf, (size_t)(2U + len));
    }
    return result;
}

int trxvu_cmd_trs_clear_beacon(trxvu_ctx_t *ctx)
{
    return trxvu_write(ctx, TRXVU_TRS_ADDR, TRXVU_CC_TRS_CLEAR_BEACON, NULL, 0U);
}

int trxvu_cmd_trs_set_idle(trxvu_ctx_t *ctx, uint8_t enable)
{
    uint8_t v = (enable != 0U) ? 1U : 0U;
    return trxvu_write(ctx, TRXVU_TRS_ADDR, TRXVU_CC_TRS_SET_IDLE, &v, 1U);
}

int trxvu_cmd_trs_telemetry(trxvu_ctx_t *ctx, uint8_t out_buf[16])
{
    int n;
    int result = -1;

    if (out_buf == NULL)
    {
        errno = EINVAL;
    }
    else
    {
        n = trxvu_write_read(ctx, TRXVU_TRS_ADDR, TRXVU_CC_TRS_TELEMETRY, NULL, 0U, out_buf, 16U);
        if (n > 0)
        {
            result = n;
        }
    }
    return result;
}

int trxvu_cmd_trs_last_trans_tlm(trxvu_ctx_t *ctx, uint8_t out_buf[16])
{
    int n;
    int result = -1;

    if (out_buf == NULL)
    {
        errno = EINVAL;
    }
    else
    {
        n = trxvu_write_read(ctx, TRXVU_TRS_ADDR, TRXVU_CC_TRS_LAST_TRANS_TLM,
                             NULL, 0U, out_buf, 16U);
        if (n > 0)
        {
            result = n;
        }
    }
    return result;
}

int trxvu_cmd_trs_set_bitrate(trxvu_ctx_t *ctx, uint8_t bitrate_mask)
{
    return trxvu_write(ctx, TRXVU_TRS_ADDR, TRXVU_CC_TRS_SET_BITRATE, &bitrate_mask, 1U);
}

int trxvu_cmd_trs_set_pll_power(trxvu_ctx_t *ctx, uint16_t pll_value)
{
    uint8_t data[2];
    data[0] = (uint8_t)(pll_value & 0xFFU);
    data[1] = (uint8_t)((pll_value >> 8) & 0xFFU);
    return trxvu_write(ctx, TRXVU_TRS_ADDR, TRXVU_CC_TRS_SET_PLL_POWER, data, sizeof(data));
}

int trxvu_cmd_trs_get_state(trxvu_ctx_t *ctx, uint8_t *out_state)
{
    int     result = -1;
    uint8_t rx[1]  = {0U};
    int     n;

    if (out_state == NULL)
    {
        errno = EINVAL;
    }
    else
    {
        n = trxvu_write_read(ctx, TRXVU_TRS_ADDR, TRXVU_CC_TRS_GET_STATE,
                             NULL, 0U, rx, sizeof(rx));
        if (n == (int)sizeof(rx))
        {
            *out_state = rx[0];
            result = 0;
        }
    }
    return result;
}

int trxvu_cmd_trs_get_pa_overtemp(trxvu_ctx_t *ctx, uint8_t *out_flag)
{
    int     result = -1;
    uint8_t rx[1]  = {0U};
    int     n;

    if (out_flag == NULL)
    {
        errno = EINVAL;
    }
    else
    {
        n = trxvu_write_read(ctx, TRXVU_TRS_ADDR, TRXVU_CC_TRS_GET_PA_OVERTEMP,
                             NULL, 0U, rx, sizeof(rx));
        if (n == (int)sizeof(rx))
        {
            *out_flag = rx[0];
            result = 0;
        }
    }
    return result;
}

int trxvu_cmd_trs_clear_pa_overtemp(trxvu_ctx_t *ctx)
{
    return trxvu_write(ctx, TRXVU_TRS_ADDR, TRXVU_CC_TRS_CLEAR_PA_OVERTEMP, NULL, 0U);
}

/* ============================================================
 * AX.25 callsign 명령 (ICD-0001 §7.3.1.2/4/6/7/8/9)
 * ============================================================ */

int trxvu_cmd_trs_get_to_callsign(trxvu_ctx_t *ctx, uint8_t out_cs[TRXVU_CALLSIGN_LEN])
{
    int result = -1;
    int n;
    if (out_cs == NULL)
    {
        errno = EINVAL;
    }
    else
    {
        n = trxvu_write_read(ctx, TRXVU_TRS_ADDR, TRXVU_CC_TRS_GET_AX25_TO_CALLSIGN,
                             NULL, 0U, out_cs, TRXVU_CALLSIGN_LEN);
        if (n == (int)TRXVU_CALLSIGN_LEN)
        {
            result = 0;
        }
    }
    return result;
}

int trxvu_cmd_trs_get_from_callsign(trxvu_ctx_t *ctx, uint8_t out_cs[TRXVU_CALLSIGN_LEN])
{
    int result = -1;
    int n;
    if (out_cs == NULL)
    {
        errno = EINVAL;
    }
    else
    {
        n = trxvu_write_read(ctx, TRXVU_TRS_ADDR, TRXVU_CC_TRS_GET_AX25_FROM_CALLSIGN,
                             NULL, 0U, out_cs, TRXVU_CALLSIGN_LEN);
        if (n == (int)TRXVU_CALLSIGN_LEN)
        {
            result = 0;
        }
    }
    return result;
}

int trxvu_cmd_trs_set_to_callsign(trxvu_ctx_t *ctx, const uint8_t cs[TRXVU_CALLSIGN_LEN])
{
    int result;
    if (cs == NULL)
    {
        errno = EINVAL;
        result = -1;
    }
    else
    {
        result = trxvu_write(ctx, TRXVU_TRS_ADDR, TRXVU_CC_TRS_SET_AX25_TO_CALLSIGN,
                             cs, TRXVU_CALLSIGN_LEN);
    }
    return result;
}

int trxvu_cmd_trs_set_from_callsign(trxvu_ctx_t *ctx, const uint8_t cs[TRXVU_CALLSIGN_LEN])
{
    int result;
    if (cs == NULL)
    {
        errno = EINVAL;
        result = -1;
    }
    else
    {
        result = trxvu_write(ctx, TRXVU_TRS_ADDR, TRXVU_CC_TRS_SET_AX25_FROM_CALLSIGN,
                             cs, TRXVU_CALLSIGN_LEN);
    }
    return result;
}

int trxvu_cmd_trs_send_ax25_override(trxvu_ctx_t *ctx,
                                     const uint8_t to_cs[TRXVU_CALLSIGN_LEN],
                                     const uint8_t from_cs[TRXVU_CALLSIGN_LEN],
                                     const uint8_t *frame, uint8_t frame_len,
                                     uint8_t *out_remaining)
{
    int     result = -1;
    uint8_t buf[14U + 235U];          /* 7 + 7 + max frame 235 */
    uint8_t rx[1] = {0U};
    int     n;

    if ((to_cs == NULL) || (from_cs == NULL) || (frame == NULL)
        || (frame_len == 0U) || (frame_len > 235U) || (out_remaining == NULL))
    {
        errno = EINVAL;
    }
    else
    {
        /* ICD-0001 §7.3.1.2: payload = to_cs(7) + from_cs(7) + frame */
        (void)memcpy(&buf[0],  to_cs,   TRXVU_CALLSIGN_LEN);
        (void)memcpy(&buf[7],  from_cs, TRXVU_CALLSIGN_LEN);
        (void)memcpy(&buf[14], frame,   frame_len);

        n = trxvu_write_read(ctx, TRXVU_TRS_ADDR, TRXVU_CC_TRS_SEND_AX25_OVERRIDE,
                             buf, (size_t)(14U + frame_len),
                             rx, sizeof(rx));
        if (n == (int)sizeof(rx))
        {
            *out_remaining = rx[0];
            result = 0;
        }
    }
    return result;
}

int trxvu_cmd_trs_set_ax25_beacon_override(trxvu_ctx_t *ctx, uint16_t interval_s,
                                           const uint8_t to_cs[TRXVU_CALLSIGN_LEN],
                                           const uint8_t from_cs[TRXVU_CALLSIGN_LEN],
                                           const uint8_t *frame, uint8_t frame_len)
{
    int     result = -1;
    uint8_t buf[16U + 235U];

    if ((to_cs == NULL) || (from_cs == NULL) || (frame == NULL)
        || (frame_len == 0U) || (frame_len > 235U))
    {
        errno = EINVAL;
    }
    else
    {
        /* ICD-0001 §7.3.1.4: [001-002] xxxxxxxx 0000xxxx + to(7) + from(7) + frame
         * byte[1] upper nibble must be 0 */
        buf[0]  = (uint8_t)(interval_s & 0xFFU);
        buf[1]  = (uint8_t)((interval_s >> 8) & 0x0FU);
        (void)memcpy(&buf[2],  to_cs,   TRXVU_CALLSIGN_LEN);
        (void)memcpy(&buf[9],  from_cs, TRXVU_CALLSIGN_LEN);
        (void)memcpy(&buf[16], frame,   frame_len);

        result = trxvu_write(ctx, TRXVU_TRS_ADDR, TRXVU_CC_TRS_SET_AX25_BEACON_OVR,
                             buf, (size_t)(16U + frame_len));
    }
    return result;
}

/* ============================================================
 * ICD-0002 Transponder Mode
 * ============================================================ */

int trxvu_cmd_set_tx_mode(trxvu_ctx_t *ctx, uint8_t mode)
{
    int result;

    if ((mode != TRXVU_TX_MODE_NOMINAL) && (mode != TRXVU_TX_MODE_TRANSPONDER))
    {
        errno = EINVAL;
        result = -1;
    }
    else
    {
        result = trxvu_write(ctx, TRXVU_TRS_ADDR, TRXVU_CC_SET_TX_MODE, &mode, 1U);
    }
    return result;
}

int trxvu_cmd_get_trp_rssi(trxvu_ctx_t *ctx, uint16_t *out_rssi_raw)
{
    int     result = -1;
    uint8_t rx[2]  = {0U};
    int     n;

    if (out_rssi_raw == NULL)
    {
        errno = EINVAL;
    }
    else
    {
        n = trxvu_write_read(ctx, TRXVU_TRS_ADDR, TRXVU_CC_GET_TRP_RSSI,
                             NULL, 0U, rx, sizeof(rx));
        if (n == (int)sizeof(rx))
        {
            /* ICD-0002 §5.1.1.2 / ICD-0001 §7 ADC raw 컨벤션 (LE):
             *   [000-001] xxxxxxxx 0000xxxx
             *   12-bit RSSI = (MSB lower-nibble << 8) | LSB byte */
            *out_rssi_raw = (uint16_t)(((uint16_t)(rx[1] & 0x0FU) << 8)
                                       | (uint16_t)rx[0]);
            result = 0;
        }
    }
    return result;
}

int trxvu_cmd_set_trp_rssi_thr(trxvu_ctx_t *ctx, uint16_t threshold_raw)
{
    int     result;
    uint8_t data[2];

    if (threshold_raw > 4095U)
    {
        errno = EINVAL;
        result = -1;
    }
    else
    {
        /* ICD-0002 §5.1.1.3: big-endian */
        data[0] = (uint8_t)((threshold_raw >> 8) & 0xFFU);
        data[1] = (uint8_t)(threshold_raw & 0xFFU);
        result = trxvu_write(ctx, TRXVU_TRS_ADDR, TRXVU_CC_SET_TRP_RSSI_THR,
                             data, sizeof(data));
    }
    return result;
}

/* ============================================================
 * DEV-001 양쪽 호출 — raw CC 인자
 * ============================================================ */

int trxvu_cmd_set_trp_freq_via(trxvu_ctx_t *ctx, uint8_t raw_cc, uint32_t freq_khz)
{
    uint8_t data[4];

    /* ICD-0002 §5.1.1.4: little-endian */
    data[0] = (uint8_t)(freq_khz & 0xFFU);
    data[1] = (uint8_t)((freq_khz >> 8)  & 0xFFU);
    data[2] = (uint8_t)((freq_khz >> 16) & 0xFFU);
    data[3] = (uint8_t)((freq_khz >> 24) & 0xFFU);

    return trxvu_write(ctx, TRXVU_TRS_ADDR, raw_cc, data, sizeof(data));
}

int trxvu_cmd_get_trp_freq_via(trxvu_ctx_t *ctx, uint8_t raw_cc,
                               uint32_t *out_freq_khz, uint8_t *out_status)
{
    int     result = -1;
    uint8_t rx[5]  = {0U};
    int     n;

    if ((out_freq_khz == NULL) || (out_status == NULL))
    {
        errno = EINVAL;
    }
    else
    {
        n = trxvu_write_read(ctx, TRXVU_TRS_ADDR, raw_cc, NULL, 0U, rx, sizeof(rx));
        if (n == (int)sizeof(rx))
        {
            *out_freq_khz = (uint32_t)rx[0]
                          | ((uint32_t)rx[1] << 8)
                          | ((uint32_t)rx[2] << 16)
                          | ((uint32_t)rx[3] << 24);
            *out_status = rx[4];
            result = 0;
        }
    }
    return result;
}
