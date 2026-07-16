#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <pthread.h>
#include <stdarg.h>
#include <time.h>
#include "i2c_lib.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define I2C_BASE_DELAY 2 // 2 milliseconds
#define I2C_MAX_RETRIES 3
#define I2C_TIMEOUT_MS 1000 // 1 second timeout

#define I2C_SUCCESS 0
#define I2C_ERR_CHANNEL -1
#define I2C_ERR_MUTEX -2
#define I2C_ERR_DEVICE -3
#define I2C_ERR_SLAVE -4
#define I2C_ERR_WRITE -5
#define I2C_ERR_READ -6
#define I2C_ERR_WRITE_READ -7

#define NUM_I2C_CHANNELS 2

const char *I2C_DEVICE[NUM_I2C_CHANNELS] = {"/dev/i2c-0", "/dev/i2c-1"};
const char *I2C_MUTEX_NAME[NUM_I2C_CHANNELS] = {"I2C_Mutex0", "I2C_Mutex1"};
pthread_mutex_t I2C_mutex[NUM_I2C_CHANNELS];

uint32_t I2C_ErrorCount_Channel = 0;
uint32_t I2C_ErrorCount_Device = 0;
uint32_t I2C_ErrorCount_Slave = 0;
uint32_t I2C_ErrorCount_Write = 0;
uint32_t I2C_ErrorCount_Read = 0;
uint32_t I2C_ErrorCount_WriteCnt = 0;
uint32_t I2C_ErrorCount_ReadCnt = 0;

void I2C_Close(void);
int I2C_LibInit(void);
void I2C_FileClose(int fd, uint8_t channel);
int I2C_Open(uint8_t channel, uint8_t devAddr);
int I2C_Check(uint8_t channel, uint8_t devAddr);
int I2C_WriteBytes(uint8_t channel, uint8_t devAddr, uint8_t cmd, uint8_t length, void *data);
int I2C_ReadBytes(uint8_t channel, uint8_t devAddr, uint8_t length, void *data);
int I2C_WriteReadBytes(uint8_t channel, uint8_t devAddr, uint8_t cmd, uint8_t wLength, void *wData, uint8_t rLength, void *rData, uint8_t delay);

void I2C_Log(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    printf("%s\n", buffer);
    va_end(args);
}

int I2C_LibInit(void)
{
    for (int i = 0; i < NUM_I2C_CHANNELS; i++)
    {
        if (pthread_mutex_init(&I2C_mutex[i], NULL) != 0)
        {
            I2C_Log("Failed to initialize I2C lib: Mutex creation failed for index %d", i);
            return -1;
        }
    }

    I2C_Log("I2C Lib Initialized.");
    atexit(I2C_Close);
    return 0;
}

void I2C_Close(void)
{
    for (int i = 0; i < NUM_I2C_CHANNELS; i++)
    {
        if (pthread_mutex_destroy(&I2C_mutex[i]) != 0)
        {
            I2C_Log("Failed to delete mutex for I2C channel %d", i);
        }
        else
        {
            I2C_Log("Successfully deleted mutex for I2C channel %d", i);
        }
    }
    I2C_Log("I2C Lib Closed");
}

void I2C_FileClose(int fd, uint8_t channel)
{
    close(fd);
    pthread_mutex_unlock(&I2C_mutex[channel]);
}

int I2C_Open(uint8_t channel, uint8_t devAddr)
{
    if (channel >= NUM_I2C_CHANNELS)
    {
        I2C_ErrorCount_Channel++;
        I2C_Log("Invalid I2C channel: %d", channel);
        return I2C_ERR_CHANNEL;
    }

    if (pthread_mutex_lock(&I2C_mutex[channel]) != 0)
    {
        I2C_Log("Failed to take mutex for channel %d", channel);
        return I2C_ERR_MUTEX;
    }

    int fd = open(I2C_DEVICE[channel], O_RDWR);
    if (fd < 0)
    {
        I2C_Log("Failed to open device: %s, devAddr: %d, error: %s", I2C_DEVICE[channel], devAddr, strerror(errno));
        I2C_ErrorCount_Device++;
        pthread_mutex_unlock(&I2C_mutex[channel]);
        return I2C_ERR_DEVICE;
    }

    if (ioctl(fd, I2C_SLAVE, devAddr) < 0)
    {
        I2C_Log("Failed to select device(%d): %s", devAddr, strerror(errno));
        I2C_ErrorCount_Slave++;
        I2C_FileClose(fd, channel);
        return I2C_ERR_SLAVE;
    }

    return fd;
}

int I2C_Check(uint8_t channel, uint8_t devAddr)
{
    int fd = I2C_Open(channel, devAddr);
    if (fd < 0)
        return fd;
    I2C_FileClose(fd, channel);
    return I2C_SUCCESS;
}

int I2C_WriteBytes(uint8_t channel, uint8_t devAddr, uint8_t cmd, uint8_t length, void *data)
{
    uint8_t buf[240];
    int count = 0;
    int retries = 0;

    while (retries < I2C_MAX_RETRIES)
    {
        int fd = I2C_Open(channel, devAddr);
        if (fd < 0)
            return fd;

        usleep(I2C_BASE_DELAY * 1000);

        buf[0] = cmd;
        memcpy(buf + 1, data, length);

        count = write(fd, buf, length + 1);
        if (count == length + 1)
        {
            I2C_FileClose(fd, channel);
            return I2C_SUCCESS;
        }

        I2C_Log("Write failed, retrying (%d/%d)", retries + 1, I2C_MAX_RETRIES);
        I2C_FileClose(fd, channel);
        retries++;
        usleep(I2C_BASE_DELAY * (retries + 1) * 1000);
    }

    I2C_ErrorCount_Write++;
    I2C_Log("Failed to write device after %d retries", I2C_MAX_RETRIES);
    return I2C_ERR_WRITE;
}

int I2C_ReadBytes(uint8_t channel, uint8_t devAddr, uint8_t length, void *data)
{
    int count = 0;
    int retries = 0;

    while (retries < I2C_MAX_RETRIES)
    {
        int fd = I2C_Open(channel, devAddr);
        if (fd < 0)
            return fd;

        usleep(I2C_BASE_DELAY * 1000);

        count = read(fd, data, length);
        if (count == length)
        {
            I2C_FileClose(fd, channel);
            return count;
        }

        I2C_Log("Read failed, retrying (%d/%d)", retries + 1, I2C_MAX_RETRIES);
        I2C_FileClose(fd, channel);
        retries++;
        usleep(I2C_BASE_DELAY * (retries + 1) * 1000);
    }

    I2C_ErrorCount_Read++;
    I2C_Log("Failed to read device after %d retries", I2C_MAX_RETRIES);
    return I2C_ERR_READ;
}

int I2C_WriteReadBytes(uint8_t channel, uint8_t devAddr, uint8_t cmd, uint8_t wLength, void *wData, uint8_t rLength, void *rData, uint8_t delay)
{
    int writeResult, readResult;
    int retries = 0;

    while (retries < I2C_MAX_RETRIES)
    {
        writeResult = I2C_WriteBytes(channel, devAddr, cmd, wLength, wData);
        if (writeResult == I2C_SUCCESS)
        {
            usleep(delay * 1000);
            readResult = I2C_ReadBytes(channel, devAddr, rLength, rData);
            if (readResult == rLength)
            {
                return readResult;
            }
        }

        I2C_Log("WriteRead failed, retrying (%d/%d)", retries + 1, I2C_MAX_RETRIES);
        retries++;
        usleep(I2C_BASE_DELAY * (retries + 1) * 1000);
    }

    I2C_ErrorCount_Write++;
    I2C_ErrorCount_Read++;
    I2C_Log("Failed to WriteRead device after %d retries", I2C_MAX_RETRIES);
    return I2C_ERR_WRITE_READ;
}