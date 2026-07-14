#ifndef _eps_functions_h_
#define _eps_functions_h_

#include "stdint.h"
#include "stdio.h"
#define _EPS_ITSDL 1

#define EPS_DEV_ADDR 0x2B

#define ITSDL_EPSBAT 1U
#define EPS
// #define BAT
#define ALL
/*
**  EPS TLE Code
*/
//  Common
#define EPS_TLE_IPCM12V                                                        \
  0xE234 //  Output Current of 12V Bus       0.00207 x ADC Count A
#define EPS_TLE_VPCM12V                                                        \
  0xE230 //  Output Voltage of 12V Bus       0.01349 x ADC Count V
#define EPS_TLE_IPCMBATV                                                       \
  0xE224 //  Output Current of Battery Bus   0.00681988679 x ADC Count A
#define EPS_TLE_VPCMBATV                                                       \
  0xE220 //  Output Voltage of Battery Bus   0.008978 x ADC Count V
#define EPS_TLE_IPCM5V                                                         \
  0xE214 //  Output Current of 5V Bus        0.00681988679 x ADC Count A
#define EPS_TLE_VPCM5V                                                         \
  0xE210 //  Output Voltage of 5V Bus        0.005865 x ADC Count V
#define EPS_TLE_IPCM3V3                                                        \
  0xE204 //  Output Current of 3.3V Bus      0.00681988679 x ADC Count A
#define EPS_TLE_VPCM3V3                                                        \
  0xE200 //  Output Voltage of 3.3V Bus      0.004311 x ADC Count V
#define EPS_TLE_VSW1                                                           \
  0xE410 //  Output Voltage Switch 1         0.01349 x ADC Count V
#define EPS_TLE_ISW1                                                           \
  0xE414 //  Output Current Switch 1         0.001328 x ADC Count A
#define EPS_TLE_VSW2                                                           \
  0xE420 //  Output Voltage Switch 2         0.01349 x ADC Count V
#define EPS_TLE_ISW2                                                           \
  0xE424 //  Output Current Switch 2         0.001328 x ADC Count A
#define EPS_TLE_VSW3                                                           \
  0xE430 //  Output Voltage Switch 3         0.008993 x ADC Count V
#define EPS_TLE_ISW3                                                           \
  0xE434 //  Output Current Switch 3         0.006239 x ADC Count A
#define EPS_TLE_VSW4                                                           \
  0xE440 //  Output Voltage Switch 4         0.008993 x ADC Count V
#define EPS_TLE_ISW4                                                           \
  0xE444 //  Output Current Switch 4         0.006239 x ADC Count A
#define EPS_TLE_VSW5                                                           \
  0xE450 //  Output Voltage Switch 5         0.005865 x ADC Count V
#define EPS_TLE_ISW5                                                           \
  0xE454 //  Output Current Switch 5         0.001328 x ADC Count A
#define EPS_TLE_VSW6                                                           \
  0xE460 //  Output Voltage Switch 6         0.005865 x ADC Count V
#define EPS_TLE_ISW6                                                           \
  0xE464 //  Output Current Switch 6         0.001328 x ADC Count A
#define EPS_TLE_VSW8                                                           \
  0xE480 //  Output Voltage Switch 8         0.004311 x ADC Count V
#define EPS_TLE_ISW8                                                           \
  0xE484 //  Output Current Switch 8         0.001328 x ADC Count A
#define EPS_TLE_VSW9                                                           \
  0xE490 //  Output Voltage Switch 9         0.004311 x ADC Count V
#define EPS_TLE_ISW9                                                           \
  0xE494 //  Output Current Switch 9         0.001328 x ADC Count A
#define EPS_TLE_TBRD                                                           \
  0xE308 //  Motherboard Temperature         (0.372434 x ADC Count) -273.15 °C

#ifdef ITSDL_EPSBAT
#define EPS_TLE_TLM_TBRD_DB                                                    \
  0xE388 //  Daughterboard Temperature       (0.372434 x ADC Count) -273.15 °C
#endif

#ifdef BAT
#define BAT_DEV_ADDR 0x2A

/*
**  Battery TLE Code
*/
#define BAT_TLM_VBAT                                                           \
  0xE280 //  Battery Output Voltage          0.008993 x ADC V     10W 20W 30W
         //  40W
#define BAT_TLM_IBAT                                                           \
  0xE284 //  Battery Current Magnitude       14.662757 x ADC mA    10W 20W 30W
         //  40W
#define BAT_TLM_IDIRBAT                                                        \
  0xE28E //  Battery Current Direction       ADC < 512 Charging; Else
         //  Discharging  -     10W 20W 30W 40W
#define BAT_TLM_TBRD                                                           \
  0xE308 //  Motherboard Temperature         (0.372434 x ADC) -273.15 °C    10W
         //  20W 30W 40W
#define BAT_TLM_IPCM5V                                                         \
  0xE214 //  Current Draw of 5V Bus          1.327547 x ADC mA    10W 20W 30W
         //  40W
#define BAT_TLM_VPCM5V                                                         \
  0xE210 //  Output Voltage of 5V Bus        0.005865 x ADC V     10W 20W 30W
         //  40W
#define BAT_TLM_IPCM3V3                                                        \
  0xE204 //  Current Draw of 3.3V Bus        1.327547 x ADC mA    10W 20W 30W
         //  40W
#define BAT_TLM_VPCM3V3                                                        \
  0xE200 //  Output Voltage of 3.3V Bus      0.004311 x ADC V     10W 20W 30W
         //  40W
#define BAT_TLM_TBAT1                                                          \
  0xE398 //  Daughterboard 1 Temperature     (0.397600 x ADC) -238.57 °C    10W
         //  20W 30W 40W
#define BAT_TLM_HBAT1                                                          \
  0xE39F //  Daughterboard 1 Heater Status   ADC < 512 Heater Off; Else On. -
         //  10W 20W 30W 40W
#define BAT_TLM_TBAT2                                                          \
  0xE3A8 //  Daughterboard 2 Temperature     (0.397600 x ADC) -238.57 °C     -
         //  20W 30W 40W
#define BAT_TLM_HBAT2                                                          \
  0xE3AF //  Daughterboard 2 Heater Status   ADC < 512 Heater Off; Else On. - -
         //  20W 30W 40W
#define BAT_TLM_TBAT3                                                          \
  0xE3B8 //  Daughterboard 3 Temperature     (0.397600 x ADC) -238.57 °C     -
         //  -  30W 40W
#define BAT_TLM_HBAT3                                                          \
  0xE3BF //  Daughterboard 3 Heater Status   ADC < 512 Heater Off; Else On. - -
         //  -  30W 40W
#define BAT_TLM_TBAT4                                                          \
  0xE3C8 //  Daughterboard 4 Temperature     (0.397600 x ADC) -238.57 °C     -
         //  -   -  40W
#define BAT_TLM_HBAT4                                                          \
  0xE3CF //  Daughterboard 4 Heater Status   ADC < 512 Heater Off; Else On. - -
         //  -   -  40W

//  Get Telemetry Data
uint16_t BAT_GetTelemetry(uint16_t TLECode);

//  Battery
uint8_t BAT_GetHeaterStatus(void);
uint8_t BAT_SetHeaterStatus(uint8_t status);
uint8_t BAT_ManualReset(void);
#endif

uint16_t EPS_GetBoardStatus(void);

uint16_t EPS_GetLastError(void);

uint16_t EPS_GetVersion(void);

uint16_t EPS_GetChecksum(void);

uint16_t EPS_GetRevision(void);

//  Get Telemetry Data
uint16_t EPS_GetTelemetry(uint16_t TLECode);

uint16_t EPS_GetNumberOfBrownOutReset(void);

uint16_t EPS_GetNumberOfAutomaticReset(void);

uint16_t EPS_GetNumberOfManuelReset(void);

uint16_t EPS_GetNumberOfCommWatchdogReset(void);

uint8_t EPS_SwitchAllPDMOn(void);

uint8_t EPS_SwitchAllPDMOff(void);

uint16_t EPS_GetActualStateOfAllPDM(void);

uint16_t EPS_GetExpectedStateOfAllPDM(void);

uint16_t EPS_GetInitialStateOfAllPDM(void);

uint8_t EPS_SetAllPDMToInitialState(void);

uint8_t EPS_SwitchNPDMOn(uint8_t channel);

uint8_t EPS_SwitchNPDMOff(uint8_t channel);

uint8_t EPS_SetInitialStateOfNPDMOn(uint8_t channel);

uint8_t EPS_SetInitialStateOfNPDMOff(uint8_t channel);

uint16_t EPS_GetActualStateOfNPDM(uint8_t channel);

//  Reset, WatchDog.
uint8_t EPS_GetNumberFromChannel(uint16_t channel);

/* 지원 PDM 채널(1~6, 8, 9)인지 확인 */
uint8_t EPS_IsValidPDMChannel(uint8_t channel);

uint8_t EPS_GetPDMSwitch(uint16_t channel);
#endif /* _eps_functions_h_ */
