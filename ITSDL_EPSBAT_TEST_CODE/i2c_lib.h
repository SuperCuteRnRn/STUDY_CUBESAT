/*************************************************************************
** File:
**   $Id: i2c_lib.h $
**
** Purpose:
**    CFS library for i2c communication
**
*************************************************************************/
#ifndef _i2c_lib_h_
#define _i2c_lib_h_

/*************************************************************************
** Includes
*************************************************************************/
// #include "cfe.h"
#include "stdint.h"

#define I2C_SUCCESS        0
#define I2C_ERR_CHANNEL    -1
#define I2C_ERR_MUTEX      -2
#define I2C_ERR_DEVICE     -3
#define I2C_ERR_SLAVE      -4
#define I2C_ERR_WRITE      -5
#define I2C_ERR_READ       -6
#define I2C_ERR_WRITE_READ -7

#define int32 int32_t
#define uint8 uint8_t
#define uint16 uint16_t
#define uint32 uint32_t

/*************************************************************************
** Public Function Prototypes
*************************************************************************/
// int32 I2C_Check(uint8 channel, uint8 devAddr);
// int32 I2C_WriteBytes(uint8 channel, uint8 devAddr, uint8 cmd, uint8 length, void *data);
// int32 I2C_ReadBytes(uint8 channel, uint8 devAddr, uint8 length, void *data);
// int32 I2C_WriteReadBytes(uint8 channel, uint8 devAddr, uint8 cmd, uint8 wLength, void *wData, uint8 rLength,
//                          void *rData, uint8 delay);

void  I2C_Close(void);
int32 I2C_LibInit(void);
void  I2C_FileClose(int32 fd, uint8 channel);
int32 I2C_Open(uint8 channel, uint8 devAddr);
int32 I2C_Check(uint8 channel, uint8 devAddr);
int32 I2C_WriteBytes(uint8 channel, uint8 devAddr, uint8 cmd, uint8 length, void *data);
int32 I2C_ReadBytes(uint8 channel, uint8 devAddr, uint8 length, void *data);
int32 I2C_WriteReadBytes(uint8 channel, uint8 devAddr, uint8 cmd, uint8 wLength, void *wData, uint8 rLength,
                         void *rData, uint8 delay);

// I2C_Library.h에 함수 선언 추가
extern uint8 I2C_ErrorCount_Channel;
extern uint8 I2C_ErrorCount_Device;
extern uint8 I2C_ErrorCount_Slave;
extern uint8 I2C_ErrorCount_Write;
extern uint8 I2C_ErrorCount_Read;
extern uint8 I2C_ErrorCount_WriteCnt;
extern uint8 I2C_ErrorCount_ReadCnt;

#endif /* _i2c_lib_h_ */

/************************/
/*  End of File Comment */
/************************/
