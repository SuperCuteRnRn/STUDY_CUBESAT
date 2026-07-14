#include "function.h"
#include "i2c_lib.h"
#define EPS_I2C_CHANNEL 1

#define EPS_GET_BOARD_STATUS 0x01
#define EPS_GET_LASTERROR 0x03
#define EPS_GET_VERSION 0x04
#define EPS_GET_CHECKSUM 0x05
#define EPS_GET_REVISION 0x06
#define EPS_GET_TELEMETRY 0x10
#define EPS_GET_NUMBER_BROWN_RESET 0x31
#define EPS_GET_NUMBER_AUTO_SOFT_RESETS 0x32
#define EPS_GET_NUMBER_MANUAL_RESETS 0x33
#define EPS_GET_NUMBER_COMM_WATCHDOG_RESETS 0x34
#define EPS_SWITCH_ON_ALL_PDMS 0x40
#define EPS_SWITCH_OFF_ALL_PDMS 0x41
#define EPS_GET_ACTUAL_STATE_ALL_PDMS 0x42
#define EPS_GET_EXPECTED_STATE_ALL_PDMS 0x43
#define EPS_GET_INITIAL_STATE_ALL_PDMS 0x44
#define EPS_SET_ALL_PDMS_TO_INITIAL_STATE 0x45
#define EPS_SET_PDM_N_ON 0x50
#define EPS_SET_PDM_N_OFF 0x51
#define EPS_SET_PDM_N_INITIAL_STATE_TO_ON 0x52
#define EPS_SET_PDM_N_INITIAL_STATE_TO_OFF 0x53
#define EPS_GET_PDM_N_ACTUAL_STATUS 0x54

uint8_t EPS_cmdBuf[4];
uint8_t EPS_dataBuf[20];
/* ITSDL BAT 보드(STM32 펌웨어)의 GET_TELEMETRY busy 윈도우는 15ms.
 * 마진을 두어 20ms 사용 (busy 중 Read 시 0xFFFF가 반환됨). */
#define EPS_DELAY 20
/*******************************************************************************
**
**  Global Functions
**
*******************************************************************************/

/* 지원 PDM 채널: 1/2=12V, 3/4=BAT, 5/6=5V, 8/9=3.3V */
uint8_t EPS_IsValidPDMChannel(uint8_t channel) {
  return (channel >= 1 && channel <= 10 && channel != 7 && channel != 10) ? 1
                                                                          : 0;
}

#ifdef _EPS_ITSDL
uint8_t EPS_GetNumberFromChannel(uint16_t channel) {
  int number;
  uint16_t switch_flag;

  for (number = 1; number <= 10; number++) {
    switch_flag = 0x0001 << number;

    if (channel & switch_flag) {
      return number;
    }
  }

  return 0;
}
#endif

// #define EPS_TLE_VPCM12V             0xE230
// #define EPS_TLE_VPCMBATV            0xE220
// #define EPS_TLE_VPCM5V              0xE210
// #define EPS_TLE_VPCM3V3             0xE200
uint16_t EPS_GetPCMVoltage(uint8_t Volt) {
  uint16_t res = 0xffff;

#ifdef _EPS_ITSDL
  res = EPS_GetTelemetry(EPS_TLE_VPCM3V3 + (16 * Volt));
#endif

  return res;
} /* EPS_GetPCMVoltage() */

// #define EPS_TLE_VSW1                0xE410
// #define EPS_TLE_VSW2                0xE420
// #define EPS_TLE_VSW3                0xE430
// #define EPS_TLE_VSW4                0xE440
// #define EPS_TLE_VSW5                0xE450
// #define EPS_TLE_VSW6                0xE460
// #define EPS_TLE_VSW7                0xE470
// #define EPS_TLE_VSW8                0xE480
// #define EPS_TLE_VSW9                0xE490
// #define EPS_TLE_VSW10               0xE4A0
uint16_t EPS_GetPDMVoltage(uint16_t channel) {
  uint16_t res = 0xffff;

#ifdef _EPS_ITSDL
  int index;
  uint16_t switch_flag;

  for (index = 1; index <= 10; index++) {
    switch_flag = 0x0001 << index;

    if (channel & switch_flag) {
      if (!EPS_IsValidPDMChannel(index)) /* 지원 채널 확인 */
        return res;
      res = EPS_GetTelemetry(EPS_TLE_VSW1 + (16 * (index - 1)));
      return res;
    }
  }

#endif

  return res;
} /* EPS_GetPDMVoltage() */

uint8_t EPS_GetPDMSwitch(uint16_t channel) {
  uint8_t res = 0xff;
  uint16_t PDMState;

  PDMState = EPS_GetExpectedStateOfAllPDM();

  if (PDMState & (channel >> 1))
    res = 1;
  else
    res = 0;

  return res;
} /* EPS_GetPDMSwitch() */

/*******************************************************************************
**
**  Local Functions
**
*******************************************************************************/
uint16_t GetPDMStateFlag(uint8_t *data) {
  uint16_t res = 0;

  res = data[2];
  res = (res << 7) | (data[3] >> 1);

  return res;
}

/*******************************************************************************
**
**  Telemetry Functions
**
*******************************************************************************/

/******************************************************************************
**  Function:  EPS_GetTelemetry()
**
**  Purpose:
**      EPS���� TLECode�� �ش��ϴ� Telemetry Data�� �������� �Լ�.
**
**      Return ���� 10bit ADC Result�� ���
*/
#ifdef _EPS_ITSDL

#endif

/*******************************************************************************
**
**  ITSDL_EPS&BAT Functions
**
*******************************************************************************/

/******************************************************************************
**  Function:  EPS_GetBoardStatus()
**
**  Purpose:
**      EPS Board Status
*/
uint16_t EPS_GetBoardStatus(void) {
  uint16_t res = 0xffff;

  EPS_cmdBuf[0] = 0;

  /* 응답: [0, Mother, 0, Daughter] → 상위=Mother, 하위=Daughter */
  if (I2C_WriteReadBytes(EPS_I2C_CHANNEL, EPS_DEV_ADDR, EPS_GET_BOARD_STATUS, 1,
                         EPS_cmdBuf, 4, EPS_dataBuf, 2) == 4) {
    res = EPS_dataBuf[1];
    res = (res << 8) | EPS_dataBuf[3];
  }

  return res;
} /* EPS_GetBoardStatus() */

/******************************************************************************
**  Function:  EPS_GetLastError()
**
**  Purpose:
**      EPS Board Status
*/
uint16_t EPS_GetLastError(void) {
  uint16_t res = 0xffff;

  EPS_cmdBuf[0] = 0;

  /* 응답: [M_hi, M_lo, D_hi, D_lo] → Motherboard word 반환 */
  if (I2C_WriteReadBytes(EPS_I2C_CHANNEL, EPS_DEV_ADDR, EPS_GET_LASTERROR, 1,
                         EPS_cmdBuf, 4, EPS_dataBuf, 2) == 4) {
    res = EPS_dataBuf[0];
    res = (res << 8) | EPS_dataBuf[1];
  }

  return res;
} /* EPS_GetLastError() */

/******************************************************************************
**  Function:  EPS_GetVersion()
**
**  Purpose:
**      EPS Board Status
*/
uint16_t EPS_GetVersion(void) {
  uint16_t res = 0xffff;

  EPS_cmdBuf[0] = 0;

  /* 응답: [Ver_hi, Ver_lo, 0, 0] */
  if (I2C_WriteReadBytes(EPS_I2C_CHANNEL, EPS_DEV_ADDR, EPS_GET_VERSION, 1,
                         EPS_cmdBuf, 4, EPS_dataBuf, 2) == 4) {
    res = EPS_dataBuf[0];
    res = (res << 8) | EPS_dataBuf[1];
  }

  return res;
} /* EPS_GetVersion() */

/******************************************************************************
**  Function:  EPS_GetChecksum()
**
**  Purpose:
**      EPS Board Status
*/
uint16_t EPS_GetChecksum(void) {
  uint16_t res = 0xffff;

  EPS_cmdBuf[0] = 0;

  /* 응답: [0, Chk_hi, 0, Chk_lo] → (b1<<8)|b3 로 16bit 복원.
   * ROM 체크섬 계산 busy 윈도우 70ms → 딜레이 75ms. */
  if (I2C_WriteReadBytes(EPS_I2C_CHANNEL, EPS_DEV_ADDR, EPS_GET_CHECKSUM, 1,
                         EPS_cmdBuf, 4, EPS_dataBuf, 75) == 4) {
    res = EPS_dataBuf[1];
    res = (res << 8) | EPS_dataBuf[3];
  }

  return res;
} /* EPS_GetChecksum() */

/******************************************************************************
**  Function:  EPS_GetRevision()
**
**  Purpose:
**      EPS Board Status
*/
uint16_t EPS_GetRevision(void) {
  uint16_t res = 0xffff;

  EPS_cmdBuf[0] = 0;

  /* 응답: [0, Rev_hi, 0, Rev_lo] → (b1<<8)|b3 로 16bit 복원 */
  if (I2C_WriteReadBytes(EPS_I2C_CHANNEL, EPS_DEV_ADDR, EPS_GET_REVISION, 1,
                         EPS_cmdBuf, 4, EPS_dataBuf, 2) == 4) {
    res = EPS_dataBuf[1];
    res = (res << 8) | EPS_dataBuf[3];
  }

  return res;
} /* EPS_GetRevision() */

uint16_t EPS_GetTelemetry(uint16_t TLECode) {
  uint16_t res = 0xffff;

  EPS_cmdBuf[0] = (TLECode & 0xff00) >> 8;
  EPS_cmdBuf[1] = (TLECode & 0x00ff);

  if (I2C_WriteReadBytes(EPS_I2C_CHANNEL, EPS_DEV_ADDR, EPS_GET_TELEMETRY, 2,
                         EPS_cmdBuf, 2, EPS_dataBuf, EPS_DELAY) == 2) {
    res = EPS_dataBuf[0];
    res = (res << 8) | EPS_dataBuf[1];
  }

  return res;
} /* EPS_GetTelemetry() */

/******************************************************************************
**  Function:  EPS_GetNumberOfBrownOutReset()
**
**  Purpose:
**      Reset�� �߻��� Ƚ���� ��������
*�Լ�.
*�ٸ�.
*/
uint16_t EPS_GetNumberOfBrownOutReset(void) {
  uint16_t res = 0xffff;

  EPS_cmdBuf[0] = 0;

#ifdef ITSDL_EPSBAT
  /* 응답: [Cnt_hi, Cnt_lo, 0, 0] */
  if (I2C_WriteReadBytes(EPS_I2C_CHANNEL, EPS_DEV_ADDR,
                         EPS_GET_NUMBER_BROWN_RESET, 1, EPS_cmdBuf, 4,
                         EPS_dataBuf, 2) == 4) {
    res = EPS_dataBuf[0];
    res = (res << 8) | EPS_dataBuf[1];
  }
#else
  if (I2C_WriteReadBytes(EPS_I2C_CHANNEL, EPS_DEV_ADDR,
                         EPS_GET_NUMBER_BROWN_RESET, 1, EPS_cmdBuf, 2,
                         EPS_dataBuf, 1) == 2) {
    res = EPS_dataBuf[0];
    res = (res << 8) | EPS_dataBuf[1];
  }
#endif

  return res;
} /* EPS_GetNumberOfBrownOutReset() */

/******************************************************************************
**  Function:  EPS_GetNumberOfAutomaticReset()
**
**  Purpose:
**      Reset�� �߻��� Ƚ���� ��������
*�Լ�.
*�ٸ�.
*/
uint16_t EPS_GetNumberOfAutomaticReset(void) {
  uint16_t res = 0xffff;

  EPS_cmdBuf[0] = 0;

#ifdef ITSDL_EPSBAT
  /* 응답: [0, Mother_cnt, 0, Daughter_cnt] → Mother 카운트 반환 */
  if (I2C_WriteReadBytes(EPS_I2C_CHANNEL, EPS_DEV_ADDR,
                         EPS_GET_NUMBER_AUTO_SOFT_RESETS, 1, EPS_cmdBuf, 4,
                         EPS_dataBuf, 2) == 4) {
    res = EPS_dataBuf[1];
  }
#else
  if (I2C_WriteReadBytes(EPS_I2C_CHANNEL, EPS_DEV_ADDR,
                         EPS_GET_NUMBER_AUTO_SOFT_RESETS, 1, EPS_cmdBuf, 2,
                         EPS_dataBuf, 1) == 2) {
    res = EPS_dataBuf[0];
    res = (res << 8) | EPS_dataBuf[1];
  }
#endif

  return res;
} /* EPS_GetNumberOfAutomaticReset() */

/******************************************************************************
**  Function:  EPS_GetNumberOfManuelReset()
**
**  Purpose:
**      Reset�� �߻��� Ƚ���� ��������
*�Լ�.
*�ٸ�.
*/
uint16_t EPS_GetNumberOfManuelReset(void) {
  uint16_t res = 0xffff;

  EPS_cmdBuf[0] = 0;

#ifdef ITSDL_EPSBAT
  /* 응답: [Cnt_hi, Cnt_lo, 0, 0] */
  if (I2C_WriteReadBytes(EPS_I2C_CHANNEL, EPS_DEV_ADDR,
                         EPS_GET_NUMBER_MANUAL_RESETS, 1, EPS_cmdBuf, 4,
                         EPS_dataBuf, 2) == 4) {
    res = EPS_dataBuf[0];
    res = (res << 8) | EPS_dataBuf[1];
  }
#else
  if (I2C_WriteReadBytes(EPS_I2C_CHANNEL, EPS_DEV_ADDR,
                         EPS_GET_NUMBER_MANUAL_RESETS, 1, EPS_cmdBuf, 2,
                         EPS_dataBuf, 1) == 2) {
    res = EPS_dataBuf[0];
    res = (res << 8) | EPS_dataBuf[1];
  }
#endif

  return res;
} /* EPS_GetNumberOfManuelReset() */

/******************************************************************************
**  Function:  EPS_GetNumberOfCommWatchdogReset()
**
**  Purpose:
**      Communication WatchDog Reset�� �߻��� Ƚ����
*�������� �Լ�.
**      0 ~ 255 ���� Return.
*/
uint16_t EPS_GetNumberOfCommWatchdogReset(void) {
  uint16_t res = 0xffff;

  EPS_cmdBuf[0] = 0;

  if (I2C_WriteReadBytes(EPS_I2C_CHANNEL, EPS_DEV_ADDR,
                         EPS_GET_NUMBER_COMM_WATCHDOG_RESETS, 1, EPS_cmdBuf, 2,
                         EPS_dataBuf, 1) == 2) {
    res = EPS_dataBuf[0];
    res = (res << 8) | EPS_dataBuf[1];
  }

  return res;
} /* EPS_GetNumberOfCommWatchdogReset() */

/******************************************************************************
**  Function:  EPS_SwitchAllPDMOn()
**
**  Purpose:
**      all PDMs switch on
*/
uint8_t EPS_SwitchAllPDMOn(void) {
  EPS_cmdBuf[0] = 0;

  return I2C_WriteBytes(EPS_I2C_CHANNEL, EPS_DEV_ADDR, EPS_SWITCH_ON_ALL_PDMS,
                        1, EPS_cmdBuf);
} /* EPS_SwitchAllPDMOn() */

/******************************************************************************
**  Function:  EPS_SwitchAllPDMOff()
**
**  Purpose:
**      all PDMs switch off
*/
uint8_t EPS_SwitchAllPDMOff(void) {
  EPS_cmdBuf[0] = 0;

  return I2C_WriteBytes(EPS_I2C_CHANNEL, EPS_DEV_ADDR, EPS_SWITCH_OFF_ALL_PDMS,
                        1, EPS_cmdBuf);
} /* EPS_SwitchAllPDMOff() */

/******************************************************************************
**  Function:  EPS_GetActualStateOfAllPDM()
**
**  Purpose:
**      actual state of all the PDMs (0 : off and 1 : on)
*/
uint16_t EPS_GetActualStateOfAllPDM(void) {
  uint16_t res = 0xffff;

  EPS_cmdBuf[0] = 0;

  if (I2C_WriteReadBytes(EPS_I2C_CHANNEL, EPS_DEV_ADDR,
                         EPS_GET_ACTUAL_STATE_ALL_PDMS, 1, EPS_cmdBuf, 4,
                         EPS_dataBuf, 20) == 4) {
    res = GetPDMStateFlag(EPS_dataBuf);
  }

  return res;
} /* EPS_GetActualStateOfAllPDM() */

/******************************************************************************
**  Function:  EPS_GetExpectedStateOfAllPDM()
**
**  Purpose:
**      expected state of all the PDMs (0 : off and 1 : on)
*/
uint16_t EPS_GetExpectedStateOfAllPDM(void) {
  uint16_t res = 0xffff;

  EPS_cmdBuf[0] = 0;

  if (I2C_WriteReadBytes(EPS_I2C_CHANNEL, EPS_DEV_ADDR,
                         EPS_GET_EXPECTED_STATE_ALL_PDMS, 1, EPS_cmdBuf, 4,
                         EPS_dataBuf, 1) == 4) {
    res = GetPDMStateFlag(EPS_dataBuf);
  }

  return res;
} /* EPS_GetExpectedStateOfAllPDM() */

/******************************************************************************
**  Function:  EPS_GetInitialStateOfAllPDM()
**
**  Purpose:
**      initial state of the PDMs
**      1 indicating the PDM is selected to be ON at power up or reset
**
*/
uint16_t EPS_GetInitialStateOfAllPDM(void) {
  uint16_t res = 0xffff;

  EPS_cmdBuf[0] = 0;

  /* 펌웨어 busy 윈도우 20ms */
  if (I2C_WriteReadBytes(EPS_I2C_CHANNEL, EPS_DEV_ADDR,
                         EPS_GET_INITIAL_STATE_ALL_PDMS, 1, EPS_cmdBuf, 4,
                         EPS_dataBuf, 20) == 4) {
    res = GetPDMStateFlag(EPS_dataBuf);
  }

  return res;
} /* EPS_GetInitialStateOfAllPDM() */

/******************************************************************************
**  Function:  EPS_SetAllPDMToInitialState()
**
**  Purpose:
**      sets the initial state of the PDMs after power on or reset
*/
uint8_t EPS_SetAllPDMToInitialState(void) {
  EPS_cmdBuf[0] = 0;

  return I2C_WriteBytes(EPS_I2C_CHANNEL, EPS_DEV_ADDR,
                        EPS_SET_ALL_PDMS_TO_INITIAL_STATE, 1, EPS_cmdBuf);
} /* EPS_SetAllPDMToInitialState() */

/******************************************************************************
**  Function:  EPS_SwitchNPDMOn()
**
**  Purpose:
**      turns on an individual PDM
*/
uint8_t EPS_SwitchNPDMOn(uint8_t channel) {
  if (!EPS_IsValidPDMChannel(channel))
    return 0xff;
  EPS_cmdBuf[0] = channel;

  return I2C_WriteBytes(EPS_I2C_CHANNEL, EPS_DEV_ADDR, EPS_SET_PDM_N_ON, 1,
                        EPS_cmdBuf);
} /* EPS_SwitchNPDMOn() */

/******************************************************************************
**  Function:  EPS_SwitchNPDMOff()
**
**  Purpose:
**      turns off an individual PDM
*/
uint8_t EPS_SwitchNPDMOff(uint8_t channel) {
  if (!EPS_IsValidPDMChannel(channel))
    return 0xff;
  EPS_cmdBuf[0] = channel;

  return I2C_WriteBytes(EPS_I2C_CHANNEL, EPS_DEV_ADDR, EPS_SET_PDM_N_OFF, 1,
                        EPS_cmdBuf);
} /* EPS_SwitchNPDMOff() */

/******************************************************************************
**  Function:  EPS_SetInitialStateOfNPDMOn()
**
**  Purpose:
**      a PDM��s initial status to be set to ON
*/
uint8_t EPS_SetInitialStateOfNPDMOn(uint8_t channel) {
  if (!EPS_IsValidPDMChannel(channel))
    return 0xff;
  EPS_cmdBuf[0] = channel;

  return I2C_WriteBytes(EPS_I2C_CHANNEL, EPS_DEV_ADDR,
                        EPS_SET_PDM_N_INITIAL_STATE_TO_ON, 1, EPS_cmdBuf);
} /* EPS_SetInitialStateOfNPDMOn() */

/******************************************************************************
**  Function:  EPS_SetInitialStateOfNPDMOff()
**
**  Purpose:
**      a PDM��s initial status to be set to OFF
*/
uint8_t EPS_SetInitialStateOfNPDMOff(uint8_t channel) {
  if (!EPS_IsValidPDMChannel(channel))
    return 0xff;
  EPS_cmdBuf[0] = channel;

  return I2C_WriteBytes(EPS_I2C_CHANNEL, EPS_DEV_ADDR,
                        EPS_SET_PDM_N_INITIAL_STATE_TO_OFF, 1, EPS_cmdBuf);
} /* EPS_SetInitialStateOfNPDMOff() */

/******************************************************************************
**  Function:  EPS_GetActualStateOfNPDM()
**
**  Purpose:
**      returns the actual state of the requested PDM (0 : off and 1 : on)
*/
uint16_t EPS_GetActualStateOfNPDM(uint8_t channel) {
  uint16_t res = 0xffff;

  if (!EPS_IsValidPDMChannel(channel))
    return res;
  EPS_cmdBuf[0] = channel;

  /* 응답: [0, 상태(0/1)] 2바이트 — 비트맵 아님 */
  if (I2C_WriteReadBytes(EPS_I2C_CHANNEL, EPS_DEV_ADDR,
                         EPS_GET_PDM_N_ACTUAL_STATUS, 1, EPS_cmdBuf, 2,
                         EPS_dataBuf, 2) == 2) {
    res = EPS_dataBuf[0];
    res = (res << 8) | EPS_dataBuf[1];
  }

  return res;
} /* EPS_GetActualStateOfNPDM() */
