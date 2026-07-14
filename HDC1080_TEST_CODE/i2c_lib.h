#ifndef I2C_LIB_H
#define I2C_LIB_H

#include <stdint.h>

#define I2C_SUCCESS 0
#define I2C_ERR_CHANNEL -1
#define I2C_ERR_MUTEX -2
#define I2C_ERR_DEVICE -3
#define I2C_ERR_SLAVE -4
#define I2C_ERR_WRITE -5
#define I2C_ERR_READ -6
#define I2C_ERR_WRITE_READ -7

#define NUM_I2C_CHANNELS 2

extern uint32_t I2C_ErrorCount_Channel;
extern uint32_t I2C_ErrorCount_Device;
extern uint32_t I2C_ErrorCount_Slave;
extern uint32_t I2C_ErrorCount_Write;
extern uint32_t I2C_ErrorCount_Read;
extern uint32_t I2C_ErrorCount_WriteCnt;
extern uint32_t I2C_ErrorCount_ReadCnt;

/**
 * @brief Initialize the I2C library
 * @return 0 on success, -1 on failure
 */
int I2C_LibInit(void);

/**
 * @brief Close the I2C library and clean up resources
 */
void I2C_Close(void);

/**
 * @brief Check if an I2C device is accessible
 * @param channel I2C channel number
 * @param devAddr I2C device address
 * @return I2C_SUCCESS on success, error code on failure
 */
int I2C_Check(uint8_t channel, uint8_t devAddr);

/**
 * @brief Write bytes to an I2C device
 * @param channel I2C channel number
 * @param devAddr I2C device address
 * @param cmd Command byte
 * @param length Number of bytes to write
 * @param data Pointer to data to write
 * @return I2C_SUCCESS on success, error code on failure
 */
int I2C_WriteBytes(uint8_t channel, uint8_t devAddr, uint8_t cmd, uint8_t length, void *data);

/**
 * @brief Read bytes from an I2C device
 * @param channel I2C channel number
 * @param devAddr I2C device address
 * @param length Number of bytes to read
 * @param data Pointer to buffer to store read data
 * @return Number of bytes read on success, error code on failure
 */
int I2C_ReadBytes(uint8_t channel, uint8_t devAddr, uint8_t length, void *data);

/**
 * @brief Write to and then read from an I2C device
 * @param channel I2C channel number
 * @param devAddr I2C device address
 * @param cmd Command byte
 * @param wLength Number of bytes to write
 * @param wData Pointer to data to write
 * @param rLength Number of bytes to read
 * @param rData Pointer to buffer to store read data
 * @param delay Delay in milliseconds between write and read operations
 * @return Number of bytes read on success, error code on failure
 */
int I2C_WriteReadBytes(uint8_t channel, uint8_t devAddr, uint8_t cmd, uint8_t wLength, void *wData, uint8_t rLength, void *rData, uint8_t delay);

/**
 * @brief Log a message with variable arguments
 * @param format Format string
 * @param ... Variable arguments
 */
void I2C_Log(const char *format, ...);

#endif // I2C_LIB_H