/******************************************************************************
 * txu.h — ITSDL_TRXVU 통신보드 flat API (공개 헤더)
 *
 * 교육용 큐브위성 통신보드(ITSDL_TRXVU)를 open/close·context 관리 없이
 * 함수 호출만으로 제어하는 초보자 친화 라이브러리.
 *   #include "txu.h"
 * 링크: gcc myapp.c -L. -ltxu -o myapp   (또는 libtxu.so 를 ctypes 로 호출)
 *
 * - Linux i2c-dev(/dev/i2c-1) 사용. slave: RCV 0x60, TRS 0x61
 * - I2C 열기는 첫 호출 시 라이브러리 내부에서 자동(lazy). TXU_Close()로 종료 가능.
 * - 반환 규약:
 *     · 명령형 함수: 0 = 성공, -1 = 오류
 *     · 스칼라 조회 함수: 값 직접 반환, 오류 = 전비트 1 (0xFFFF / 0xFFFFFFFF)
 *     · 버퍼/다중값 함수: out 파라미터 사용, 반환 0/-1 (또는 읽은 byte 수)
 * - 외부 프레임워크 무의존. gcc + libc + linux/i2c-dev.h 만 필요.
 *
 * (!) 위험 명령 경고 — RF 송출/영구 변경/보드 리셋 유발. 호출 전 운용 규제·안전 절차 확인:
 *     TXU_TrsSendFrame / TXU_TrsSendFrameOverride            (RF 송출)
 *     TXU_TrsSetBeacon / TXU_TrsSetBeaconOverride            (RF 반복 송출)
 *     TXU_SetFrequency / TXU_SetTransponderFrequency         (주파수 변경)
 *     TXU_TrsSetToCallsign / TXU_TrsSetFromCallsign          (콜사인 영구 변경)
 *     TXU_WatchdogReset / TXU_SoftwareReset / TXU_HardwareReset (보드 리셋)
 ******************************************************************************/
#ifndef TXU_H
#define TXU_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── slave 선택 ─────────────────────────────────────────────────────────── */
typedef enum
{
    TXU_RCV = 0,   /* Receiver    section (0x60) */
    TXU_TRS = 1    /* Transmitter section (0x61) */
} TXU_Slave;

/* ── 상수 ───────────────────────────────────────────────────────────────── */
#define TXU_CALLSIGN_LEN        7U      /* 6 ASCII + 1 SSID */

/* Set TX mode 값 (TXU_SetTxMode) */
#define TXU_TX_MODE_NOMINAL     0x01U
#define TXU_TX_MODE_TRANSPONDER 0x02U

/* Bitrate 마스크 (TXU_TrsSetBitrate) */
#define TXU_BITRATE_1200        0x01U
#define TXU_BITRATE_2400        0x02U
#define TXU_BITRATE_4800        0x04U
#define TXU_BITRATE_9600        0x08U

/* 스칼라 오류 sentinel */
#define TXU_ERR_U16             0xFFFFU
#define TXU_ERR_U32             0xFFFFFFFFU

/* ── 라이프사이클 (선택 — 대부분 호출 불필요) ───────────────────────────── */
/* 첫 명령 전에 기본 "/dev/i2c-1" 이 아닌 경로를 쓰려면 호출. 열기 전에만 유효. */
void TXU_SetDevice(const char *i2c_device_path);
/* 명시적 open (성공 0 / 실패 -1). 생략해도 첫 명령에서 자동 open. */
int  TXU_Open(void);
/* 명시적 close. 프로그램 종료 시 호출 권장(생략 가능). */
void TXU_Close(void);

/* ── 공통 명령 (RCV/TRS 양쪽) ──────────────────────────────────────────── */
uint32_t TXU_GetUptime(TXU_Slave slave);                 /* 초, 오류 0xFFFFFFFF */
uint32_t TXU_GetFrequency(TXU_Slave slave);              /* kHz, 오류 0xFFFFFFFF */
int      TXU_SetFrequency(TXU_Slave slave, uint32_t freq_khz);       /* (!) */
int      TXU_GetPllError(TXU_Slave slave, uint16_t *out_lock_err,
                         uint16_t *out_freq_err);
int      TXU_GetFirmwareInfo(TXU_Slave slave, char out_buf[80]);
int      TXU_GetLastResetCause(TXU_Slave slave, uint8_t *out_cause);

/* ── RX (Receiver 0x60) ────────────────────────────────────────────────── */
int  TXU_RcvGetTelemetry(uint8_t out_buf[16]);           /* 반환 byte 수 / -1 */
int  TXU_RcvGetFrameCount(uint16_t *out_count);
int  TXU_RcvGetFrameLength(uint16_t *out_len);
int  TXU_RcvGetFrame(uint8_t *out_frame, size_t max_len, size_t *out_actual);
int  TXU_RcvRemoveFrame(void);
int  TXU_RcvRemoveAllFrames(void);

/* ── TX (Transmitter 0x61) ─────────────────────────────────────────────── */
int  TXU_TrsGetTelemetry(uint8_t out_buf[16]);           /* 반환 byte 수 / -1 */
int  TXU_TrsGetLastTxTelemetry(uint8_t out_buf[16]);
int  TXU_TrsGetState(uint8_t *out_state);
int  TXU_TrsSetIdle(uint8_t enable);
int  TXU_TrsSetBitrate(uint8_t bitrate_mask);
int  TXU_TrsSetPllPower(uint16_t pll_value);
int  TXU_TrsGetPaOverTemp(uint8_t *out_flag);
int  TXU_TrsClearPaOverTemp(void);
int  TXU_TrsClearBeacon(void);
int  TXU_TrsSendFrame(const uint8_t *data, uint8_t len, uint8_t *out_remaining); /* (!) */
int  TXU_TrsSetBeacon(uint16_t interval_s, const uint8_t *data, uint8_t len);    /* (!) */

/* AX.25 콜사인 (7 byte = 6 ASCII + 1 SSID) */
int  TXU_TrsGetToCallsign(uint8_t out_cs[TXU_CALLSIGN_LEN]);
int  TXU_TrsGetFromCallsign(uint8_t out_cs[TXU_CALLSIGN_LEN]);
int  TXU_TrsSetToCallsign(const uint8_t cs[TXU_CALLSIGN_LEN]);     /* (!) 영구 */
int  TXU_TrsSetFromCallsign(const uint8_t cs[TXU_CALLSIGN_LEN]);   /* (!) 영구 */
int  TXU_TrsSendFrameOverride(const uint8_t to_cs[TXU_CALLSIGN_LEN],
                              const uint8_t from_cs[TXU_CALLSIGN_LEN],
                              const uint8_t *frame, uint8_t frame_len,
                              uint8_t *out_remaining);             /* (!) RF */
int  TXU_TrsSetBeaconOverride(uint16_t interval_s,
                              const uint8_t to_cs[TXU_CALLSIGN_LEN],
                              const uint8_t from_cs[TXU_CALLSIGN_LEN],
                              const uint8_t *frame, uint8_t frame_len); /* (!) RF */

/* ── Transponder (ICD-0002, FM 전용) ───────────────────────────────────── */
int  TXU_SetTxMode(uint8_t mode);                        /* TXU_TX_MODE_* */
int  TXU_GetTransponderRssi(uint16_t *out_rssi_raw);
int  TXU_SetTransponderRssiThreshold(uint16_t threshold_raw);
int  TXU_SetTransponderFrequency(uint32_t freq_khz);                 /* (!) */
int  TXU_GetTransponderFrequency(uint32_t *out_freq_khz, uint8_t *out_status);

/* ── Reset (위험) ──────────────────────────────────────────────────────── */
int  TXU_WatchdogReset(TXU_Slave slave);                             /* (!) */
int  TXU_SoftwareReset(TXU_Slave slave);                             /* (!) */
int  TXU_HardwareReset(TXU_Slave slave);                             /* (!) */

/* ── DEV-001 (transponder freq 코드 자동 비교 시험) ───────────────────── */
int  TXU_RunDev001Test(void);   /* 0=결정, -1=baseline 실패, -2=ambiguous */

#ifdef __cplusplus
}
#endif

#endif /* TXU_H */
