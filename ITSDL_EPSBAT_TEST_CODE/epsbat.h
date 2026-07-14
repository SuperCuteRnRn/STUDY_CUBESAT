/******************************************************************************
 * epsbat.h — ITSDL_EPS&BAT 전원 관리 보드 API (공개 헤더)
 *
 * 교육용 큐브위성 시스템을 위한 전원 관리 보드 제어 라이브러리.
 * 링크: gcc myapp.c -L. -lepsbat   (또는 libepsbat.so를 ctypes로 호출)
 *
 * - I2C 버스 1(/dev/i2c-1), 슬레이브 주소 0x2B 사용
 * - 조회 함수의 반환값 0xFFFF(또는 0xFF)는 오류/미응답을 의미
 * - 명령별 처리 시간은 라이브러리가 내부에서 자동 준수
 ******************************************************************************/
#ifndef EPSBAT_H
#define EPSBAT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── 텔레메트리(TLE) 코드 ─────────────────────────────────────────────── */
/* 전원 버스 전압/전류: 물리량 = 반환값(ADC count) x 스케일 (데이터시트 6.4절) */
#define EPS_TLE_VPCMBATV 0xE220  /* BAT 버스 전압   x0.008978  V */
#define EPS_TLE_IPCMBATV 0xE224  /* BAT 버스 전류   x0.00681989 A */
#define EPS_TLE_VPCM12V  0xE230  /* 12V 버스 전압   x0.01349   V */
#define EPS_TLE_IPCM12V  0xE234  /* 12V 버스 전류   x0.00206663 A */
#define EPS_TLE_VPCM5V   0xE210  /* 5V  버스 전압   x0.005865  V */
#define EPS_TLE_IPCM5V   0xE214  /* 5V  버스 전류   x0.00681989 A */
#define EPS_TLE_VPCM3V3  0xE200  /* 3.3V 버스 전압  x0.004311  V */
#define EPS_TLE_IPCM3V3  0xE204  /* 3.3V 버스 전류  x0.00681989 A */

/* PDM 채널: 전압 코드 = EPS_TLE_VSW1 + 16*(채널-1), 전류 코드 = 전압 코드 + 4 */
#define EPS_TLE_VSW1     0xE410
#define EPS_TLE_ISW1     0xE414

/* 보드 온도: 온도('C) = 0.372434 x 반환값 - 273.15 */
#define EPS_TLE_TBRD     0xE308  /* 보드 온도 1 */
#define EPS_TLE_TBRD_DB  0xE388  /* 보드 온도 2 */

/* ── 보드 상태/정보 ───────────────────────────────────────────────────── */
uint16_t EPS_GetBoardStatus(void);   /* 정상 0x0000 */
uint16_t EPS_GetLastError(void);     /* 0x01 명령, 0x02 데이터, 0x03 채널/TLE */
uint16_t EPS_GetVersion(void);       /* 0x0001 */
uint16_t EPS_GetChecksum(void);      /* 0xA5A5 */
uint16_t EPS_GetRevision(void);      /* 0x0001 */

/* ── 텔레메트리 ───────────────────────────────────────────────────────── */
/* 반환: 10-bit ADC count (0~1023). 0xFFFF = 오류/busy */
uint16_t EPS_GetTelemetry(uint16_t TLECode);

/* ── 리셋 카운터 (부팅 원인별 누적) ───────────────────────────────────── */
uint16_t EPS_GetNumberOfBrownOutReset(void);      /* 워치독 리셋 */
uint16_t EPS_GetNumberOfAutomaticReset(void);     /* SW 리셋 */
uint16_t EPS_GetNumberOfManuelReset(void);        /* 핀 리셋 */
uint16_t EPS_GetNumberOfCommWatchdogReset(void);  /* 전원 투입 */

/* ── PDM (부하 분배) 제어 — 지원 채널: 1~6, 8, 9 ─────────────────────── */
/* 채널 매핑: 1/2=12V, 3/4=BAT, 5/6=5V, 8/9=3.3V */
uint8_t  EPS_IsValidPDMChannel(uint8_t channel);  /* 1=지원 채널 */

uint8_t  EPS_SwitchNPDMOn(uint8_t channel);       /* 0xFF = 채널 오류 */
uint8_t  EPS_SwitchNPDMOff(uint8_t channel);
uint8_t  EPS_SwitchAllPDMOn(void);
uint8_t  EPS_SwitchAllPDMOff(void);

uint16_t EPS_GetActualStateOfNPDM(uint8_t channel);   /* 0/1, 0xFFFF=오류 */
uint16_t EPS_GetActualStateOfAllPDM(void);            /* 비트맵: bit0=채널1 */
uint16_t EPS_GetExpectedStateOfAllPDM(void);
uint16_t EPS_GetInitialStateOfAllPDM(void);

uint8_t  EPS_SetInitialStateOfNPDMOn(uint8_t channel);
uint8_t  EPS_SetInitialStateOfNPDMOff(uint8_t channel);
uint8_t  EPS_SetAllPDMToInitialState(void);

#ifdef __cplusplus
}
#endif

#endif /* EPSBAT_H */
