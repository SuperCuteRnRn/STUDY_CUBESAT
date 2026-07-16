#ifndef _trx_rcv_function_h_
#define _trx_rcv_function_h_

#include "stdbool.h"
#include "stdint.h"

#define TRX_FRAME_SIZE_UP 200U
#define TRX_FRAME_SIZE_DN 235U

#define TRX_BITRATE_1200 0x01U
#define TRX_BITRATE_2400 0x02U
#define TRX_BITRATE_4800 0x04U
#define TRX_BITRATE_9600 0x08U

#define TRX_PLL_POWER_LVL4 0xFFCFU
#define TRX_PLL_POWER_LVL5 0xEFCFU

/* RX I2C addresses (project-specific) */
#define TRX_RCV_ADDR_PRIMARY    0x60U  /* Device 1: receives Board ID 1 (DUST), 2 (GAS) */
#define TRX_RCV_ADDR_SECONDARY  0x61U  /* Device 2: receives Board ID 3 (DUST), 4 (GAS) */

#define uint8  uint8_t
#define uint16 uint16_t
#define uint32 uint32_t
#define int32  int32_t
#define int16  int16_t
#define int8   int8_t

/* ---- 텔레메트리 변환식 (ICD §7.4.1 Table 10) ---- */
#define RCV_DOPPLER_HZ(raw)  ((float)(int16_t)(raw) * 38.15f)
#define RCV_RSSI_dBm(raw)    ((float)(int16_t)(raw) * -0.5f - 22.0f)
#define BUS_VOLT_V(raw)      ((float)(raw) * 0.00488f)
#define CURR_mA(raw)         ((float)(raw) * 0.3152f)
#define TEMP_C(raw)          ((float)(raw) * (-0.07669f) + 195.6037f)

/* ---- 0x50 reset cause counter ---- */
typedef struct {
  uint8 interrupt_pedding_Cnt;
  uint8 Brownout_Cnt;
  uint8 RSTNMI_Cnt;
  uint8 PMMSWBOR_Cnt;
  uint8 Wakeup_LPM_Cnt;
  uint8 Security_BOR_Cnt;
  uint8 SVSL_Cnt;
  uint8 SVSH_Cnt;
} TRX_RCV_Reset_Cause_t;

/* ---- 0x25 length telecommands ---- */
typedef struct {
  uint16 Rcv_Telecommands_Length;
} TRX_RCV_GET_LENGTH_TELECOMMANDS_t;

/* ---- 0x1A 응답 — ICD §7.2.1.1 byte 매핑과 1:1 ---- */
typedef struct {
  uint16 instantaneous_doppler_offset;        /* [00-01] 16-bit 2's complement */
  uint16 instantaneous_signal_strength;       /* [02-03] 16-bit 2's complement */
  uint16 Supply_Voltage;                      /* [04-05] 12-bit ADC */
  uint16 Total_Supply_Current;                /* [06-07] */
  uint16 TRS_Current;                         /* [08-09] */
  uint16 RCV_Current;                         /* [10-11] */
  uint16 Power_AMP_Current;                   /* [12-13] */
  uint16 Power_AMP_Temp;                      /* [14-15] */
  uint16 Local_OSC_Temp;                      /* [16-17] */
#ifdef TRX_REVISION_E
  uint16 last_RX_doppler_offset;              /* [18-19] */
  uint16 last_RX_RSSI;                        /* [20-21] */
#endif
} TRX_RCV_Measure_Telemetry_t;

/* ---- 변환된 telemetry (float) ---- */
typedef struct {
  float instantaneous_doppler_offset_Hz;
  float instantaneous_RSSI_dBm;
  float Supply_Voltage_V;
  float Total_Supply_Current_mA;
  float TRS_Current_mA;
  float RCV_Current_mA;
  float Power_AMP_Current_mA;
  float Power_AMP_Temp_C;
  float Local_OSC_Temp_C;
#ifdef TRX_REVISION_E
  float last_RX_doppler_offset_Hz;
  float last_RX_RSSI_dBm;
#endif
} TRX_RCV_Measure_Telemetry_Trans_Function_t;

/* ---- 0x22 frame buffer (no AX.25 header) ---- */
typedef struct {
  uint16 frame_size;
  uint16 doppler_frequency;
  uint16 rssi;
  uint8  frame_contents[TRX_FRAME_SIZE_UP];
} TRX_RCS_RES_Frame_buf;

/* ---- 0x23 full frame buffer (with 18B AX.25 header) — REV_E ---- */
#ifdef TRX_REVISION_E
typedef struct {
  uint16 frame_size;
  uint16 doppler_frequency;
  uint16 rssi;
  uint8  ax25_header[18];
  uint8  frame_contents[TRX_FRAME_SIZE_UP];
} TRX_RCS_RES_Full_Frame_buf;
#endif

/* =========================================================================
 * RECEIVER FUNCTION (RCV)
 *   - 모든 함수 첫 인자: uint8_t addr (RX I2C 주소: 0x60 or 0x61)
 * ========================================================================= */
void TRX_RCV_Reset_Watchdog(uint8_t addr);
void TRX_RCV_Reset_Hardware(uint8_t addr);
void TRX_RCV_Reset_Software(uint8_t addr);

int16 TRX_RCV_Get_Frame_num(uint8_t addr, uint16 *frameNum);
void  TRX_RCV_Get_Frame(uint8_t addr, TRX_RCS_RES_Frame_buf *response);
void  TRX_RCV_Remove_Frame(uint8_t addr);

void  TRX_RCV_Measure_Telemetry(uint8_t addr, TRX_RCV_Measure_Telemetry_t *response);
void  TRX_RCV_Measure_Telemetry_Trans_Function(
          TRX_RCV_Measure_Telemetry_t *response,
          TRX_RCV_Measure_Telemetry_Trans_Function_t *res);

void  TRX_RCV_Report_Uptime(uint8_t addr, uint32 *uptime);

#ifdef TRX_REVISION_E
void  TRX_RCV_Get_Full_Frame(uint8_t addr, TRX_RCS_RES_Full_Frame_buf *response);
void  TRX_RCV_Get_Len_Command(uint8_t addr, uint16 *count, uint16 *lengths, uint16 max_count);
void  TRX_RCV_Remove_All_Frame(uint8_t addr);

void  TRX_RCV_Set_Frequency(uint8_t addr, uint32 freq);
void  TRX_RCV_Get_Frequency(uint8_t addr, uint32 *freq);
void  TRX_RCV_Get_PLL_Err_Count(uint8_t addr, uint32 *pll_err);
void  TRX_RCV_Firmware_Infor(uint8_t addr, char *firmware);  /* 80B buf */
void  TRX_RCV_Get_Last_Reset_Cause(uint8_t addr, uint16 *cause);
#endif

#endif /* _trx_rcv_function_h_ */
