#include "trx_rcv_function.h"
#include "i2c_lib.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* ---- Common ---- */
#define TRX_WATCHDOG_RESET       0xCCU
#define TRX_SOFTWARE_RESET       0xAAU
#define TRX_HARDWARE_RESET       0xABU
#define TRX_REPORT_RCVTRS_UPTIME 0x40U

#ifdef TRX_REVISION_E
#define TRX_SET_FREQUENCY        0x32U
#define TRX_GET_FREQUENCY        0x33U
#define TRX_GET_PLL_ERROR_COUNT  0x34U
#define TRX_GET_FIRMWARE_INFOR   0x42U
#define TRX_GET_LAST_RESET_CAUSE 0x50U
#define TRX_FW_STR_MAX           80U
#endif

/* ---- Receiver-specific ---- */
#define RCV_GET_FRAME_NUMBER         0x21U
#define RCV_GET_FRAME                0x22U
#define RCV_REMOVE_FRAME             0x24U
#define RCV_MEASURE_TELEMETRY        0x1AU

#ifdef TRX_REVISION_E
#define RCV_GET_FULL_FRAME           0x23U
#define RCV_GET_LENGTH_TELECOMMANDS  0x25U
#define RCV_REMOVE_ALL_FRAME         0x26U
#endif

#define TRX_CHANNEL                  1U
#define I2C_OP_DELAY_MS              10U

#define RX_FRAME_BUF    (TRX_FRAME_SIZE_UP + 6U)         /* size+doppler+rssi + payload */
#define RX_FULL_BUF     (TRX_FRAME_SIZE_UP + 6U + 18U)   /* + AX.25 header */
#define RX_LEN_LIST_BUF (2U + (40U * 2U))                /* count + 최대 40 length */

static uint16_t U16_LE(const uint8_t *p) {
  uint16_t result = 0U;
  if (p != NULL) {
    result = (uint16_t)p[0] | ((uint16_t)p[1] << 8);
  }
  return result;
}

/* =========================================================================
 * Reset
 * ========================================================================= */
void TRX_RCV_Reset_Watchdog(uint8_t addr) {
  (void)I2C_WriteBytes(TRX_CHANNEL, addr, TRX_WATCHDOG_RESET, 0U, NULL);
}
void TRX_RCV_Reset_Software(uint8_t addr) {
  (void)I2C_WriteBytes(TRX_CHANNEL, addr, TRX_SOFTWARE_RESET, 0U, NULL);
}
void TRX_RCV_Reset_Hardware(uint8_t addr) {
  (void)I2C_WriteBytes(TRX_CHANNEL, addr, TRX_HARDWARE_RESET, 0U, NULL);
}

/* =========================================================================
 * 0x21 - frame count
 * ========================================================================= */
int16 TRX_RCV_Get_Frame_num(uint8_t addr, uint16 *frameNum) {
  uint8_t buf[2] = {0};
  int16_t res = (int16_t)I2C_WriteReadBytes(
      TRX_CHANNEL, addr, RCV_GET_FRAME_NUMBER, 0U, NULL,
      (uint8_t)sizeof(buf), buf, I2C_OP_DELAY_MS);
  if ((frameNum != NULL) && (res >= 2)) {
    *frameNum = U16_LE(buf);
  }
  return res;
}

/* =========================================================================
 * 0x22 - get oldest frame
 * ========================================================================= */
void TRX_RCV_Get_Frame(uint8_t addr, TRX_RCS_RES_Frame_buf *response) {
  uint8_t buf[RX_FRAME_BUF] = {0};
  int16_t readLen = (int16_t)I2C_WriteReadBytes(
      TRX_CHANNEL, addr, RCV_GET_FRAME, 0U, NULL,
      (uint8_t)sizeof(buf), buf, I2C_OP_DELAY_MS);
  if ((response != NULL) && (readLen >= 6)) {
    uint16_t copy = 0U;
    response->frame_size        = U16_LE(&buf[0]);
    response->doppler_frequency = U16_LE(&buf[2]);
    response->rssi              = U16_LE(&buf[4]);
    copy = (uint16_t)(readLen - 6);
    if (copy > TRX_FRAME_SIZE_UP) {
      copy = TRX_FRAME_SIZE_UP;
    }
    if (response->frame_size < copy) {
      copy = response->frame_size;
    }
    (void)memcpy(response->frame_contents, &buf[6], copy);
  }
}

/* =========================================================================
 * 0x24 - remove oldest frame
 * ========================================================================= */
void TRX_RCV_Remove_Frame(uint8_t addr) {
  (void)I2C_WriteBytes(TRX_CHANNEL, addr, RCV_REMOVE_FRAME, 0U, NULL);
}

/* =========================================================================
 * 0x1A - measure all telemetry channels
 * ========================================================================= */
void TRX_RCV_Measure_Telemetry(uint8_t addr, TRX_RCV_Measure_Telemetry_t *response) {
#ifdef TRX_REVISION_E
  uint8_t buf[22] = {0};
#else
  uint8_t buf[18] = {0};
#endif
  int16_t len = (int16_t)I2C_WriteReadBytes(
      TRX_CHANNEL, addr, RCV_MEASURE_TELEMETRY, 0U, NULL,
      (uint8_t)sizeof(buf), buf, I2C_OP_DELAY_MS);
  if ((response != NULL) && (len >= (int16_t)sizeof(buf))) {
    response->instantaneous_doppler_offset  = U16_LE(&buf[0]);
    response->instantaneous_signal_strength = U16_LE(&buf[2]);
    response->Supply_Voltage                = U16_LE(&buf[4]);
    response->Total_Supply_Current          = U16_LE(&buf[6]);
    response->TRS_Current                   = U16_LE(&buf[8]);
    response->RCV_Current                   = U16_LE(&buf[10]);
    response->Power_AMP_Current             = U16_LE(&buf[12]);
    response->Power_AMP_Temp                = U16_LE(&buf[14]);
    response->Local_OSC_Temp                = U16_LE(&buf[16]);
#ifdef TRX_REVISION_E
    response->last_RX_doppler_offset        = U16_LE(&buf[18]);
    response->last_RX_RSSI                  = U16_LE(&buf[20]);
#endif
  }
}

void TRX_RCV_Measure_Telemetry_Trans_Function(
    TRX_RCV_Measure_Telemetry_t *r,
    TRX_RCV_Measure_Telemetry_Trans_Function_t *o) {
  if ((r != NULL) && (o != NULL)) {
    o->instantaneous_doppler_offset_Hz = RCV_DOPPLER_HZ(r->instantaneous_doppler_offset);
    o->instantaneous_RSSI_dBm          = RCV_RSSI_dBm(r->instantaneous_signal_strength);
    o->Supply_Voltage_V                = BUS_VOLT_V(r->Supply_Voltage);
    o->Total_Supply_Current_mA         = CURR_mA(r->Total_Supply_Current);
    o->TRS_Current_mA                  = CURR_mA(r->TRS_Current);
    o->RCV_Current_mA                  = CURR_mA(r->RCV_Current);
    o->Power_AMP_Current_mA            = CURR_mA(r->Power_AMP_Current);
    o->Power_AMP_Temp_C                = TEMP_C(r->Power_AMP_Temp);
    o->Local_OSC_Temp_C                = TEMP_C(r->Local_OSC_Temp);
#ifdef TRX_REVISION_E
    o->last_RX_doppler_offset_Hz       = RCV_DOPPLER_HZ(r->last_RX_doppler_offset);
    o->last_RX_RSSI_dBm                = RCV_RSSI_dBm(r->last_RX_RSSI);
#endif
  }
}

/* =========================================================================
 * 0x40 - uptime
 * ========================================================================= */
void TRX_RCV_Report_Uptime(uint8_t addr, uint32 *uptime) {
  (void)I2C_WriteReadBytes(TRX_CHANNEL, addr, TRX_REPORT_RCVTRS_UPTIME, 0U,
                           NULL, 4U, uptime, I2C_OP_DELAY_MS);
}

#ifdef TRX_REVISION_E
/* =========================================================================
 * 0x23 - get full frame (18B AX.25 header + payload)
 * ========================================================================= */
void TRX_RCV_Get_Full_Frame(uint8_t addr, TRX_RCS_RES_Full_Frame_buf *response) {
  uint8_t buf[RX_FULL_BUF] = {0};
  int16_t readLen = (int16_t)I2C_WriteReadBytes(
      TRX_CHANNEL, addr, RCV_GET_FULL_FRAME, 0U, NULL,
      (uint8_t)sizeof(buf), buf, I2C_OP_DELAY_MS);
  if ((response != NULL) && (readLen >= (6 + 18))) {
    uint16_t copy = 0U;
    response->frame_size        = U16_LE(&buf[0]);
    response->doppler_frequency = U16_LE(&buf[2]);
    response->rssi              = U16_LE(&buf[4]);
    (void)memcpy(response->ax25_header, &buf[6], 18U);
    copy = (uint16_t)(readLen - 24);
    if (copy > TRX_FRAME_SIZE_UP) {
      copy = TRX_FRAME_SIZE_UP;
    }
    if (response->frame_size > 18U) {
      uint16_t fs = (uint16_t)(response->frame_size - 18U);
      if (fs < copy) {
        copy = fs;
      }
    }
    (void)memcpy(response->frame_contents, &buf[24], copy);
  }
}

/* =========================================================================
 * 0x25 - get length telecommands list
 * ========================================================================= */
void TRX_RCV_Get_Len_Command(uint8_t addr, uint16 *count, uint16 *lengths, uint16 max_count) {
  uint8_t buf[RX_LEN_LIST_BUF] = {0};
  int16_t len = (int16_t)I2C_WriteReadBytes(
      TRX_CHANNEL, addr, RCV_GET_LENGTH_TELECOMMANDS, 0U, NULL,
      (uint8_t)sizeof(buf), buf, I2C_OP_DELAY_MS);
  if ((count != NULL) && (len >= 2)) {
    uint16_t n = U16_LE(&buf[0]);
    *count = n;
    if (lengths != NULL) {
      uint16_t lim = (n > max_count) ? max_count : n;
      uint16_t i = 0U;
      for (i = 0U; i < lim; i++) {
        lengths[i] = U16_LE(&buf[2U + (i * 2U)]);
      }
    }
  }
}

/* =========================================================================
 * 0x26 - remove all frames
 * ========================================================================= */
void TRX_RCV_Remove_All_Frame(uint8_t addr) {
  (void)I2C_WriteBytes(TRX_CHANNEL, addr, RCV_REMOVE_ALL_FRAME, 0U, NULL);
}

/* =========================================================================
 * 0x32 / 0x33 - frequency
 * ========================================================================= */
void TRX_RCV_Set_Frequency(uint8_t addr, uint32 freq) {
  (void)I2C_WriteBytes(TRX_CHANNEL, addr, TRX_SET_FREQUENCY, 4U, &freq);
}
void TRX_RCV_Get_Frequency(uint8_t addr, uint32 *freq) {
  (void)I2C_WriteReadBytes(TRX_CHANNEL, addr, TRX_GET_FREQUENCY, 0U,
                           NULL, 4U, freq, I2C_OP_DELAY_MS);
}

/* =========================================================================
 * 0x34 - PLL error counter
 * ========================================================================= */
void TRX_RCV_Get_PLL_Err_Count(uint8_t addr, uint32 *pll_err) {
  (void)I2C_WriteReadBytes(TRX_CHANNEL, addr, TRX_GET_PLL_ERROR_COUNT, 0U,
                           NULL, 4U, pll_err, I2C_OP_DELAY_MS);
}

/* =========================================================================
 * 0x42 - firmware info string (80B)
 * ========================================================================= */
void TRX_RCV_Firmware_Infor(uint8_t addr, char *firmware) {
  (void)I2C_WriteReadBytes(TRX_CHANNEL, addr, TRX_GET_FIRMWARE_INFOR, 0U,
                           NULL, TRX_FW_STR_MAX, firmware, I2C_OP_DELAY_MS);
}

/* =========================================================================
 * 0x50 - last reset cause
 * ========================================================================= */
void TRX_RCV_Get_Last_Reset_Cause(uint8_t addr, uint16 *cause) {
  uint8_t buf[2] = {0};
  int16_t len = (int16_t)I2C_WriteReadBytes(
      TRX_CHANNEL, addr, TRX_GET_LAST_RESET_CAUSE, 0U, NULL,
      (uint8_t)sizeof(buf), buf, I2C_OP_DELAY_MS);
  if ((cause != NULL) && (len >= 2)) {
    *cause = U16_LE(buf);
  }
}
#endif /* TRX_REVISION_E */
