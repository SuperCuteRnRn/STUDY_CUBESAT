/* ============================================================================
 * trxvu_commands.h — ITSDL_TRXVU 명령 wrapper (공통/RX/TX 26 + 트랜스폰더 5)
 * (PLAN_TRX_HW_TEST_STANDALONE_20260521.md, 2026-05-21)
 * ========================================================================== */
#ifndef TRXVU_COMMANDS_H
#define TRXVU_COMMANDS_H

#include <stddef.h>
#include <stdint.h>
#include "trxvu_i2c.h"

/* === Raw Command Codes (공통/RX/TX) === */
/* Reset */
#define TRXVU_CC_WATCHDOG_RESET     0xCCU   /* 양쪽 slave 공통 */
#define TRXVU_CC_SOFTWARE_RESET     0xAAU   /* Rev D 이전, 양쪽 slave */
#define TRXVU_CC_HARDWARE_RESET     0xABU   /* 양쪽 slave */

/* Common (양쪽 slave 모두 지원) */
#define TRXVU_CC_GET_UPTIME         0x40U
#define TRXVU_CC_SET_FREQUENCY      0x32U   /* ⚠️ RCV freq set */
#define TRXVU_CC_GET_FREQUENCY      0x33U   /* RCV freq get */
#define TRXVU_CC_GET_PLL_ERR        0x34U
#define TRXVU_CC_GET_FIRMWARE_INFO  0x42U
#define TRXVU_CC_GET_LAST_RESET     0x50U

/* RX (slave 0x60) */
#define TRXVU_CC_RCV_TELEMETRY      0x1AU
#define TRXVU_CC_RCV_GET_FRAME_NUM  0x21U
#define TRXVU_CC_RCV_GET_FRAME      0x22U
#define TRXVU_CC_RCV_GET_FULL_FRAME 0x23U
#define TRXVU_CC_RCV_REMOVE_FRAME   0x24U
#define TRXVU_CC_RCV_GET_LENGTH     0x25U
#define TRXVU_CC_RCV_REMOVE_ALL     0x26U

/* TX (slave 0x61) */
#define TRXVU_CC_TRS_SEND_FRAME             0x10U
#define TRXVU_CC_TRS_SEND_AX25_OVERRIDE     0x11U   /* Send AX.25 w/ override callsigns */
#define TRXVU_CC_TRS_SET_BEACON             0x14U
#define TRXVU_CC_TRS_SET_AX25_BEACON_OVR    0x15U   /* Set AX.25 beacon w/ override callsigns */
#define TRXVU_CC_TRS_CLEAR_BEACON           0x1FU
#define TRXVU_CC_TRS_GET_AX25_TO_CALLSIGN   0x20U   /* Get current AX.25 TO callsign */
#define TRXVU_CC_TRS_GET_AX25_FROM_CALLSIGN 0x21U   /* Get current AX.25 FROM callsign */
#define TRXVU_CC_TRS_SET_AX25_TO_CALLSIGN   0x22U   /* Set default AX.25 TO callsign */
#define TRXVU_CC_TRS_SET_AX25_FROM_CALLSIGN 0x23U   /* Set default AX.25 FROM callsign */
#define TRXVU_CC_TRS_SET_IDLE               0x24U
#define TRXVU_CC_TRS_TELEMETRY              0x25U
#define TRXVU_CC_TRS_LAST_TRANS_TLM         0x26U
#define TRXVU_CC_TRS_SET_BITRATE            0x28U
#define TRXVU_CC_TRS_SET_PLL_POWER          0x35U
#define TRXVU_CC_TRS_GET_STATE              0x41U
#define TRXVU_CC_TRS_GET_PA_OVERTEMP        0x60U
#define TRXVU_CC_TRS_CLEAR_PA_OVERTEMP      0x61U

/* AX.25 callsign: 6 ASCII + 1 SSID byte */
#define TRXVU_CALLSIGN_LEN  7U

/* === Raw Command Codes (Transponder) === */
#define TRXVU_CC_SET_TX_MODE        0x38U
#define TRXVU_CC_GET_TRP_RSSI       0x51U
#define TRXVU_CC_SET_TRP_RSSI_THR   0x52U
/* 운영 기본 코드 (0x32/0x33) */
#define TRXVU_CC_SET_TRP_FREQ       0x32U   /* transponder freq set */
#define TRXVU_CC_GET_TRP_FREQ       0x33U   /* transponder freq get */
/* 대체 코드 */
#define TRXVU_CC_SET_TRP_FREQ_ALT   0x80U   /* transponder freq set (alt) */
#define TRXVU_CC_GET_TRP_FREQ_ALT   0x81U   /* transponder freq get (alt) */

/* === Mode values for 0x38 === */
#define TRXVU_TX_MODE_NOMINAL      0x01U
#define TRXVU_TX_MODE_TRANSPONDER  0x02U

/* === Bitrate flags for 0x28 === */
#define TRXVU_BITRATE_1200   0x01U
#define TRXVU_BITRATE_2400   0x02U
#define TRXVU_BITRATE_4800   0x04U
#define TRXVU_BITRATE_9600   0x08U

/* ============================================================
 * Reset 명령 — 위험. CLI 에서 별도 확인 prompt 처리.
 * ============================================================ */
int trxvu_cmd_watchdog_reset(trxvu_ctx_t *ctx, uint8_t slave);
int trxvu_cmd_software_reset(trxvu_ctx_t *ctx, uint8_t slave);
int trxvu_cmd_hardware_reset(trxvu_ctx_t *ctx, uint8_t slave);

/* ============================================================
 * Common 명령 (RCV/TRS 양쪽)
 * ============================================================ */
int trxvu_cmd_get_uptime(trxvu_ctx_t *ctx, uint8_t slave, uint32_t *out_seconds);
int trxvu_cmd_set_frequency(trxvu_ctx_t *ctx, uint8_t slave, uint32_t freq_khz);
int trxvu_cmd_get_frequency(trxvu_ctx_t *ctx, uint8_t slave, uint32_t *out_khz);
int trxvu_cmd_get_pll_err(trxvu_ctx_t *ctx, uint8_t slave,
                          uint16_t *out_lock_err, uint16_t *out_freq_err);
int trxvu_cmd_get_firmware_info(trxvu_ctx_t *ctx, uint8_t slave, char out_buf[80]);
int trxvu_cmd_get_last_reset_cause(trxvu_ctx_t *ctx, uint8_t slave, uint8_t *out_cause);

/* ============================================================
 * RX (slave 0x60)
 * ============================================================ */
int trxvu_cmd_rcv_telemetry(trxvu_ctx_t *ctx, uint8_t out_buf[16]);
int trxvu_cmd_rcv_get_frame_num(trxvu_ctx_t *ctx, uint16_t *out_num);
int trxvu_cmd_rcv_get_length(trxvu_ctx_t *ctx, uint16_t *out_len);
int trxvu_cmd_rcv_get_frame(trxvu_ctx_t *ctx, uint8_t *out_frame, size_t max_len,
                            size_t *out_actual);
int trxvu_cmd_rcv_remove_frame(trxvu_ctx_t *ctx);
int trxvu_cmd_rcv_remove_all(trxvu_ctx_t *ctx);

/* ============================================================
 * TX (slave 0x61)
 * ============================================================ */
int trxvu_cmd_trs_send_frame(trxvu_ctx_t *ctx, const uint8_t *data, uint8_t len,
                             uint8_t *out_remaining);
int trxvu_cmd_trs_set_beacon(trxvu_ctx_t *ctx, uint16_t interval_s,
                             const uint8_t *data, uint8_t len);
int trxvu_cmd_trs_clear_beacon(trxvu_ctx_t *ctx);
int trxvu_cmd_trs_set_idle(trxvu_ctx_t *ctx, uint8_t enable);
int trxvu_cmd_trs_telemetry(trxvu_ctx_t *ctx, uint8_t out_buf[16]);
int trxvu_cmd_trs_last_trans_tlm(trxvu_ctx_t *ctx, uint8_t out_buf[16]);
int trxvu_cmd_trs_set_bitrate(trxvu_ctx_t *ctx, uint8_t bitrate_mask);
int trxvu_cmd_trs_set_pll_power(trxvu_ctx_t *ctx, uint16_t pll_value);
int trxvu_cmd_trs_get_state(trxvu_ctx_t *ctx, uint8_t *out_state);
int trxvu_cmd_trs_get_pa_overtemp(trxvu_ctx_t *ctx, uint8_t *out_flag);
int trxvu_cmd_trs_clear_pa_overtemp(trxvu_ctx_t *ctx);

/* === AX.25 callsign 명령 === */

/** @brief Get current AX.25 TO callsign (0x20). out_cs: 7 byte. */
int trxvu_cmd_trs_get_to_callsign(trxvu_ctx_t *ctx, uint8_t out_cs[TRXVU_CALLSIGN_LEN]);

/** @brief Get current AX.25 FROM callsign (0x21). out_cs: 7 byte. */
int trxvu_cmd_trs_get_from_callsign(trxvu_ctx_t *ctx, uint8_t out_cs[TRXVU_CALLSIGN_LEN]);

/** @brief Set default AX.25 TO callsign (0x22). cs: 7 byte (6 ASCII + 1 SSID). */
int trxvu_cmd_trs_set_to_callsign(trxvu_ctx_t *ctx, const uint8_t cs[TRXVU_CALLSIGN_LEN]);

/** @brief Set default AX.25 FROM callsign (0x23). cs: 7 byte. */
int trxvu_cmd_trs_set_from_callsign(trxvu_ctx_t *ctx, const uint8_t cs[TRXVU_CALLSIGN_LEN]);

/**
 * @brief Send AX.25 frame with override callsigns (0x11).
 *        Payload: to_cs(7) + from_cs(7) + frame.
 * @param out_remaining  남은 free slot 수 (응답 1 byte)
 */
int trxvu_cmd_trs_send_ax25_override(trxvu_ctx_t *ctx,
                                     const uint8_t to_cs[TRXVU_CALLSIGN_LEN],
                                     const uint8_t from_cs[TRXVU_CALLSIGN_LEN],
                                     const uint8_t *frame, uint8_t frame_len,
                                     uint8_t *out_remaining);

/**
 * @brief Set AX.25 beacon with override callsigns (0x15).
 *        Payload: interval_s(2 LE) + to_cs(7) + from_cs(7) + frame.
 */
int trxvu_cmd_trs_set_ax25_beacon_override(trxvu_ctx_t *ctx, uint16_t interval_s,
                                           const uint8_t to_cs[TRXVU_CALLSIGN_LEN],
                                           const uint8_t from_cs[TRXVU_CALLSIGN_LEN],
                                           const uint8_t *frame, uint8_t frame_len);

/* ============================================================
 * Transponder Mode
 * ============================================================ */
int trxvu_cmd_set_tx_mode(trxvu_ctx_t *ctx, uint8_t mode);
int trxvu_cmd_get_trp_rssi(trxvu_ctx_t *ctx, uint16_t *out_rssi_raw);
int trxvu_cmd_set_trp_rssi_thr(trxvu_ctx_t *ctx, uint16_t threshold_raw);

/* ============================================================
 * Transponder freq 양쪽 호출 — raw CC 인자 받음 (0x80 또는 0x32 / 0x81 또는 0x33)
 * ============================================================ */
int trxvu_cmd_set_trp_freq_via(trxvu_ctx_t *ctx, uint8_t raw_cc, uint32_t freq_khz);
int trxvu_cmd_get_trp_freq_via(trxvu_ctx_t *ctx, uint8_t raw_cc,
                               uint32_t *out_freq_khz, uint8_t *out_status);

#endif /* TRXVU_COMMANDS_H */
