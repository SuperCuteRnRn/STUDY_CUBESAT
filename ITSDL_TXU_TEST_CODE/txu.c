/******************************************************************************
 * txu.c — ITSDL_TRXVU flat API 구현
 *
 * 검증된 trxvu_cmd_* (trxvu_commands.c) 위에 context 은닉 wrapper 를 씌운다.
 * I2C 디바이스는 첫 명령에서 lazy-open, 이후 재사용한다.
 *
 * 코딩 규약: 함수당 단일 return(MISRA 15.5), 선언 시 초기화, 매직넘버 #define,
 *            싱글턴 컨텍스트는 파일 내부(static) 링크로 전역 노출 최소화.
 ******************************************************************************/
#include "txu.h"

#include "trxvu_i2c.h"
#include "trxvu_commands.h"
#include "trxvu_dev001.h"

/* 싱글턴 컨텍스트(파일 내부 링크 — 외부 전역 노출 없음) */
static trxvu_ctx_t g_txu_ctx;
static int         g_txu_opened = 0;
static char        g_txu_device[64] = TRXVU_I2C_DEFAULT_DEVICE;

/* slave enum → I2C 주소 */
static uint8_t txu_addr(TXU_Slave slave)
{
    return (slave == TXU_TRS) ? TRXVU_TRS_ADDR : TRXVU_RCV_ADDR;
}

/* 필요 시 1회 open. 성공 0 / 실패 -1. */
static int txu_ensure_open(void)
{
    int result = 0;

    if (g_txu_opened == 0)
    {
        trxvu_ctx_init(&g_txu_ctx);
        if (trxvu_open(&g_txu_ctx, g_txu_device) == 0)
        {
            g_txu_opened = 1;
        }
        else
        {
            result = -1;
        }
    }
    return result;
}

/* ── 라이프사이클 ─────────────────────────────────────────────────────── */
void TXU_SetDevice(const char *i2c_device_path)
{
    size_t i;

    if ((i2c_device_path != NULL) && (g_txu_opened == 0))
    {
        for (i = 0U; (i < (sizeof(g_txu_device) - 1U))
                     && (i2c_device_path[i] != '\0'); i++)
        {
            g_txu_device[i] = i2c_device_path[i];
        }
        g_txu_device[i] = '\0';
    }
}

int TXU_Open(void)
{
    return txu_ensure_open();
}

void TXU_Close(void)
{
    if (g_txu_opened != 0)
    {
        (void)trxvu_close(&g_txu_ctx);
        g_txu_opened = 0;
    }
}

/* ── 공통 ─────────────────────────────────────────────────────────────── */
uint32_t TXU_GetUptime(TXU_Slave slave)
{
    uint32_t result = TXU_ERR_U32;
    uint32_t sec;

    if (txu_ensure_open() == 0)
    {
        if (trxvu_cmd_get_uptime(&g_txu_ctx, txu_addr(slave), &sec) == 0)
        {
            result = sec;
        }
    }
    return result;
}

uint32_t TXU_GetFrequency(TXU_Slave slave)
{
    uint32_t result = TXU_ERR_U32;
    uint32_t khz;

    if (txu_ensure_open() == 0)
    {
        if (trxvu_cmd_get_frequency(&g_txu_ctx, txu_addr(slave), &khz) == 0)
        {
            result = khz;
        }
    }
    return result;
}

int TXU_SetFrequency(TXU_Slave slave, uint32_t freq_khz)
{
    int result = -1;

    if (txu_ensure_open() == 0)
    {
        result = trxvu_cmd_set_frequency(&g_txu_ctx, txu_addr(slave), freq_khz);
    }
    return result;
}

int TXU_GetPllError(TXU_Slave slave, uint16_t *out_lock_err, uint16_t *out_freq_err)
{
    int result = -1;

    if (txu_ensure_open() == 0)
    {
        result = trxvu_cmd_get_pll_err(&g_txu_ctx, txu_addr(slave),
                                       out_lock_err, out_freq_err);
    }
    return result;
}

int TXU_GetFirmwareInfo(TXU_Slave slave, char out_buf[80])
{
    int result = -1;

    if (txu_ensure_open() == 0)
    {
        result = trxvu_cmd_get_firmware_info(&g_txu_ctx, txu_addr(slave), out_buf);
    }
    return result;
}

int TXU_GetLastResetCause(TXU_Slave slave, uint8_t *out_cause)
{
    int result = -1;

    if (txu_ensure_open() == 0)
    {
        result = trxvu_cmd_get_last_reset_cause(&g_txu_ctx, txu_addr(slave), out_cause);
    }
    return result;
}

/* ── RX ───────────────────────────────────────────────────────────────── */
int TXU_RcvGetTelemetry(uint8_t out_buf[16])
{
    int result = -1;

    if (txu_ensure_open() == 0)
    {
        result = trxvu_cmd_rcv_telemetry(&g_txu_ctx, out_buf);
    }
    return result;
}

int TXU_RcvGetFrameCount(uint16_t *out_count)
{
    int result = -1;

    if (txu_ensure_open() == 0)
    {
        result = trxvu_cmd_rcv_get_frame_num(&g_txu_ctx, out_count);
    }
    return result;
}

int TXU_RcvGetFrameLength(uint16_t *out_len)
{
    int result = -1;

    if (txu_ensure_open() == 0)
    {
        result = trxvu_cmd_rcv_get_length(&g_txu_ctx, out_len);
    }
    return result;
}

int TXU_RcvGetFrame(uint8_t *out_frame, size_t max_len, size_t *out_actual)
{
    int result = -1;

    if (txu_ensure_open() == 0)
    {
        result = trxvu_cmd_rcv_get_frame(&g_txu_ctx, out_frame, max_len, out_actual);
    }
    return result;
}

int TXU_RcvRemoveFrame(void)
{
    int result = -1;

    if (txu_ensure_open() == 0)
    {
        result = trxvu_cmd_rcv_remove_frame(&g_txu_ctx);
    }
    return result;
}

int TXU_RcvRemoveAllFrames(void)
{
    int result = -1;

    if (txu_ensure_open() == 0)
    {
        result = trxvu_cmd_rcv_remove_all(&g_txu_ctx);
    }
    return result;
}

/* ── TX ───────────────────────────────────────────────────────────────── */
int TXU_TrsGetTelemetry(uint8_t out_buf[16])
{
    int result = -1;

    if (txu_ensure_open() == 0)
    {
        result = trxvu_cmd_trs_telemetry(&g_txu_ctx, out_buf);
    }
    return result;
}

int TXU_TrsGetLastTxTelemetry(uint8_t out_buf[16])
{
    int result = -1;

    if (txu_ensure_open() == 0)
    {
        result = trxvu_cmd_trs_last_trans_tlm(&g_txu_ctx, out_buf);
    }
    return result;
}

int TXU_TrsGetState(uint8_t *out_state)
{
    int result = -1;

    if (txu_ensure_open() == 0)
    {
        result = trxvu_cmd_trs_get_state(&g_txu_ctx, out_state);
    }
    return result;
}

int TXU_TrsSetIdle(uint8_t enable)
{
    int result = -1;

    if (txu_ensure_open() == 0)
    {
        result = trxvu_cmd_trs_set_idle(&g_txu_ctx, enable);
    }
    return result;
}

int TXU_TrsSetBitrate(uint8_t bitrate_mask)
{
    int result = -1;

    if (txu_ensure_open() == 0)
    {
        result = trxvu_cmd_trs_set_bitrate(&g_txu_ctx, bitrate_mask);
    }
    return result;
}

int TXU_TrsSetPllPower(uint16_t pll_value)
{
    int result = -1;

    if (txu_ensure_open() == 0)
    {
        result = trxvu_cmd_trs_set_pll_power(&g_txu_ctx, pll_value);
    }
    return result;
}

int TXU_TrsGetPaOverTemp(uint8_t *out_flag)
{
    int result = -1;

    if (txu_ensure_open() == 0)
    {
        result = trxvu_cmd_trs_get_pa_overtemp(&g_txu_ctx, out_flag);
    }
    return result;
}

int TXU_TrsClearPaOverTemp(void)
{
    int result = -1;

    if (txu_ensure_open() == 0)
    {
        result = trxvu_cmd_trs_clear_pa_overtemp(&g_txu_ctx);
    }
    return result;
}

int TXU_TrsClearBeacon(void)
{
    int result = -1;

    if (txu_ensure_open() == 0)
    {
        result = trxvu_cmd_trs_clear_beacon(&g_txu_ctx);
    }
    return result;
}

int TXU_TrsSendFrame(const uint8_t *data, uint8_t len, uint8_t *out_remaining)
{
    int result = -1;

    if (txu_ensure_open() == 0)
    {
        result = trxvu_cmd_trs_send_frame(&g_txu_ctx, data, len, out_remaining);
    }
    return result;
}

int TXU_TrsSetBeacon(uint16_t interval_s, const uint8_t *data, uint8_t len)
{
    int result = -1;

    if (txu_ensure_open() == 0)
    {
        result = trxvu_cmd_trs_set_beacon(&g_txu_ctx, interval_s, data, len);
    }
    return result;
}

int TXU_TrsGetToCallsign(uint8_t out_cs[TXU_CALLSIGN_LEN])
{
    int result = -1;

    if (txu_ensure_open() == 0)
    {
        result = trxvu_cmd_trs_get_to_callsign(&g_txu_ctx, out_cs);
    }
    return result;
}

int TXU_TrsGetFromCallsign(uint8_t out_cs[TXU_CALLSIGN_LEN])
{
    int result = -1;

    if (txu_ensure_open() == 0)
    {
        result = trxvu_cmd_trs_get_from_callsign(&g_txu_ctx, out_cs);
    }
    return result;
}

int TXU_TrsSetToCallsign(const uint8_t cs[TXU_CALLSIGN_LEN])
{
    int result = -1;

    if (txu_ensure_open() == 0)
    {
        result = trxvu_cmd_trs_set_to_callsign(&g_txu_ctx, cs);
    }
    return result;
}

int TXU_TrsSetFromCallsign(const uint8_t cs[TXU_CALLSIGN_LEN])
{
    int result = -1;

    if (txu_ensure_open() == 0)
    {
        result = trxvu_cmd_trs_set_from_callsign(&g_txu_ctx, cs);
    }
    return result;
}

int TXU_TrsSendFrameOverride(const uint8_t to_cs[TXU_CALLSIGN_LEN],
                             const uint8_t from_cs[TXU_CALLSIGN_LEN],
                             const uint8_t *frame, uint8_t frame_len,
                             uint8_t *out_remaining)
{
    int result = -1;

    if (txu_ensure_open() == 0)
    {
        result = trxvu_cmd_trs_send_ax25_override(&g_txu_ctx, to_cs, from_cs,
                                                  frame, frame_len, out_remaining);
    }
    return result;
}

int TXU_TrsSetBeaconOverride(uint16_t interval_s,
                             const uint8_t to_cs[TXU_CALLSIGN_LEN],
                             const uint8_t from_cs[TXU_CALLSIGN_LEN],
                             const uint8_t *frame, uint8_t frame_len)
{
    int result = -1;

    if (txu_ensure_open() == 0)
    {
        result = trxvu_cmd_trs_set_ax25_beacon_override(&g_txu_ctx, interval_s,
                                                        to_cs, from_cs, frame, frame_len);
    }
    return result;
}

/* ── Transponder ──────────────────────────────────────────────────────── */
int TXU_SetTxMode(uint8_t mode)
{
    int result = -1;

    if (txu_ensure_open() == 0)
    {
        result = trxvu_cmd_set_tx_mode(&g_txu_ctx, mode);
    }
    return result;
}

int TXU_GetTransponderRssi(uint16_t *out_rssi_raw)
{
    int result = -1;

    if (txu_ensure_open() == 0)
    {
        result = trxvu_cmd_get_trp_rssi(&g_txu_ctx, out_rssi_raw);
    }
    return result;
}

int TXU_SetTransponderRssiThreshold(uint16_t threshold_raw)
{
    int result = -1;

    if (txu_ensure_open() == 0)
    {
        result = trxvu_cmd_set_trp_rssi_thr(&g_txu_ctx, threshold_raw);
    }
    return result;
}

/* 운영 기본 코드 0x32/0x33 사용 (DEV-001 RESOLVED 2026-05-21) */
int TXU_SetTransponderFrequency(uint32_t freq_khz)
{
    int result = -1;

    if (txu_ensure_open() == 0)
    {
        result = trxvu_cmd_set_trp_freq_via(&g_txu_ctx, TRXVU_CC_SET_TRP_FREQ, freq_khz);
    }
    return result;
}

int TXU_GetTransponderFrequency(uint32_t *out_freq_khz, uint8_t *out_status)
{
    int result = -1;

    if (txu_ensure_open() == 0)
    {
        result = trxvu_cmd_get_trp_freq_via(&g_txu_ctx, TRXVU_CC_GET_TRP_FREQ,
                                            out_freq_khz, out_status);
    }
    return result;
}

/* ── Reset ────────────────────────────────────────────────────────────── */
int TXU_WatchdogReset(TXU_Slave slave)
{
    int result = -1;

    if (txu_ensure_open() == 0)
    {
        result = trxvu_cmd_watchdog_reset(&g_txu_ctx, txu_addr(slave));
    }
    return result;
}

int TXU_SoftwareReset(TXU_Slave slave)
{
    int result = -1;

    if (txu_ensure_open() == 0)
    {
        result = trxvu_cmd_software_reset(&g_txu_ctx, txu_addr(slave));
    }
    return result;
}

int TXU_HardwareReset(TXU_Slave slave)
{
    int result = -1;

    if (txu_ensure_open() == 0)
    {
        result = trxvu_cmd_hardware_reset(&g_txu_ctx, txu_addr(slave));
    }
    return result;
}

/* ── DEV-001 ──────────────────────────────────────────────────────────── */
int TXU_RunDev001Test(void)
{
    int result = -1;

    if (txu_ensure_open() == 0)
    {
        result = trxvu_dev001_run(&g_txu_ctx);
    }
    return result;
}
