/*************************************************************************
** File:
**   $Id: i2c_lib.c $
**
** Purpose:
**    CFS library for i2c communication
**
*************************************************************************/

/*************************************************************************
** Includes
*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
// #include "osapi.h"
// #include "common_types.h"
#include <linux/i2c-dev.h>
#include <stdarg.h>
#include <linux/i2c.h>
#include "i2c_lib.h"
#include "i2c_lib_version.h"

#define I2C_DEBUG_MODE  /* Enable debug logging */

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif

#define I2C_TIMEOUT_MS 5000 // 5 second timeout
/* I2C_TIMEOUT ioctl 단위는 10ms → ms를 tick으로 반올림 상향 */
#ifndef I2C_TIMEOUT_TICKS
#define I2C_TIMEOUT_TICKS ((I2C_TIMEOUT_MS + 9) / 10)
#endif

/* I2C_BASE_DELAY: milliseconds 단위 */
#define I2C_BASE_DELAY_MS 100  /* 100 milliseconds - increased for AT91/SAMA5 stability */

#define I2C_NUMBER 3
const char *I2C_DEVICE[I2C_NUMBER] = {
    "/dev/i2c-0",
    "/dev/i2c-1",
    "/dev/i2c-2",
};

static int32 I2C_fd_cache[I2C_NUMBER] = {-1, -1, -1};
static uint8 I2C_cur_addr[I2C_NUMBER] = {0};
static uint8 I2C_has_rdwr[I2C_NUMBER] = {0};

const char *I2C_MUTEX_NAME[I2C_NUMBER] = {"I2C_Mutex0", "I2C_Mutex1", "I2C_Mutex2"};
/* osal_id_t removed for standalone Linux build */



uint8 I2C_ErrorCount_Channel  = 0;
uint8 I2C_ErrorCount_Device   = 0;
uint8 I2C_ErrorCount_Slave    = 0;
uint8 I2C_ErrorCount_Write    = 0;
uint8 I2C_ErrorCount_Read     = 0;
uint8 I2C_ErrorCount_WriteCnt = 0;
uint8 I2C_ErrorCount_ReadCnt  = 0;

void  I2C_Close(void);
int32 I2C_LibInit(void);
void  I2C_FileClose(int32 fd, uint8 channel);
int32 I2C_Open(uint8 channel, uint8 devAddr);
int32 I2C_Check(uint8 channel, uint8 devAddr);
int32 I2C_WriteBytes(uint8 channel, uint8 devAddr, uint8 cmd, uint8 length, void *data);
int32 I2C_ReadBytes(uint8 channel, uint8 devAddr, uint8 length, void *data);
int32 I2C_WriteReadBytes(uint8 channel, uint8 devAddr, uint8 cmd, uint8 wLength, void *wData, uint8 rLength,
                         void *rData, uint8 delay);

void I2C_Log(const char *format, ...)
{
#ifdef I2C_DEBUG_MODE
    va_list args;
    va_start(args, format);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    fprintf(stderr, "[I2C] %s\n", buffer);
    va_end(args);
#else
    (void)format;
#endif
}
int32 I2C_LibInit(void)
{
    I2C_Log("I2C Lib Initialized. Version %d.%d.%d.%d", I2C_LIB_MAJOR_VERSION, I2C_LIB_MINOR_VERSION, I2C_LIB_REVISION,
            I2C_LIB_MISSION_REV);
    return I2C_SUCCESS;
}

void I2C_FileClose(int32 fd, uint8 channel)
{
    (void)fd; /* persistent fd */
    // OS_MutSemGive(I2C_mutex_id[channel]);
}

/* fd 캐시 + TIMEOUT/FUNCS 확인 (RETRIES 제거) */
int32 I2C_Open(uint8 channel, uint8 devAddr)
{
    if ((channel != 0) && (channel != 1) && (channel != 2))
    {
        I2C_ErrorCount_Channel++;
        I2C_Log("Invalid I2C channel: %d", channel);
        return I2C_ERR_CHANNEL;
    }

    // if (OS_MutSemTake(I2C_mutex_id[channel]) != OS_SUCCESS)
    // {
    //     I2C_Log("Failed to take mutex for channel %d", channel);
    //     return I2C_ERR_MUTEX;
    // }

    int32 fd = I2C_fd_cache[channel];
    if (fd < 0)
    {
        fd = open(I2C_DEVICE[channel], O_RDWR | O_CLOEXEC);
        if (fd < 0)
        {
            I2C_Log("Failed to open device: %s, devAddr: %d, error: %s", I2C_DEVICE[channel], devAddr, strerror(errno));
            I2C_ErrorCount_Device++;
            // OS_MutSemGive(I2C_mutex_id[channel]);
            return I2C_ERR_DEVICE;
        }

        /* 커널 타임아웃만 설정: I2C_TIMEOUT은 10ms 단위 */
        unsigned long tmo10 = (I2C_TIMEOUT_TICKS > 0) ? ((I2C_TIMEOUT_TICKS + 9) / 10) : 200; /* 기본 2s */
        if (tmo10 < 100)
            tmo10 = 100; /* 최소 1s */
        (void)ioctl(fd, I2C_TIMEOUT, tmo10);

        /* 어댑터 기능 확인(I2C_RDWR 가능 여부) */
        unsigned long funcs   = 0;
        I2C_has_rdwr[channel] = 0;
        if (ioctl(fd, I2C_FUNCS, &funcs) == 0 && (funcs & I2C_FUNC_I2C))
            I2C_has_rdwr[channel] = 1;

        /* AT91/Atmel 계열은 RDWR 비활성화 (반복 START 문제 회피) */
        int busno = -1;
        if (sscanf(I2C_DEVICE[channel], "/dev/i2c-%d", &busno) == 1)
        {
            char path[64];
            snprintf(path, sizeof(path), "/sys/class/i2c-dev/i2c-%d/name", busno);
            int nfd = open(path, O_RDONLY | O_CLOEXEC);
            if (nfd >= 0)
            {
                char    namebuf[64] = {0};
                ssize_t n           = read(nfd, namebuf, sizeof(namebuf) - 1);
                close(nfd);
                if (n > 0)
                {
                    if (strstr(namebuf, "at91") || strstr(namebuf, "AT91") || strstr(namebuf, "atmel") ||
                        strstr(namebuf, "Atmel") || strstr(namebuf, "sama5") || strstr(namebuf, "SAMA5"))
                    {
                        I2C_has_rdwr[channel] = 0;
                        I2C_Log("Adapter '%s' detected -> disable I2C_RDWR on channel %d", namebuf, channel);
                    }
                }
            }
        }

        I2C_fd_cache[channel] = fd;
    }

    if (I2C_cur_addr[channel] != devAddr)
    {
        if (ioctl(fd, I2C_SLAVE, devAddr) < 0)
        {
            I2C_Log("Failed to select device(0x%02X): %s", devAddr, strerror(errno));
            I2C_ErrorCount_Slave++;
            // OS_MutSemGive(I2C_mutex_id[channel]);
            return I2C_ERR_SLAVE;
        }
        I2C_cur_addr[channel] = devAddr;
        // OS_TaskDelay(I2C_BASE_DELAY); /* 슬레이브 전환 후 안정화 */
        usleep(I2C_BASE_DELAY_MS * 1000);
    }

    return fd;
}

void I2C_Close(void)
{
    uint8 index;
    for (index = 0; index < I2C_NUMBER; index++)
    {
        if (I2C_fd_cache[index] >= 0)
        {
            close(I2C_fd_cache[index]);
            I2C_fd_cache[index] = -1;
        }
    }
    I2C_Log("I2C Lib Closed");
}

int32 I2C_Check(uint8 channel, uint8 devAddr)
{
    int32 fd = I2C_Open(channel, devAddr);
    if (fd < 0)
        return fd;
    I2C_FileClose(fd, channel);
    return I2C_SUCCESS;
}

int32 I2C_WriteBytes(uint8 channel, uint8 devAddr, uint8 cmd, uint8 length, void *data)
{
    uint8 buf[length + 1];

    int32 fd = I2C_Open(channel, devAddr);
    if (fd < 0)
        return fd;

    // OS_TaskDelay(I2C_BASE_DELAY);
    usleep(I2C_BASE_DELAY_MS * 1000);

    buf[0] = cmd;
    memcpy(buf + 1, data, length);

    int32 count = write(fd, buf, length + 1);
    if (count == length + 1)
    {
        I2C_FileClose(fd, channel);
        return I2C_SUCCESS;
    }

    I2C_FileClose(fd, channel);
    I2C_ErrorCount_Write++;
    I2C_Log("Write failed (no retry). count=%d expected=%d errno=%d", count, length + 1, errno);
    return I2C_ERR_WRITE;
}

int32 I2C_ReadBytes(uint8 channel, uint8 devAddr, uint8 length, void *data)
{
    int32 fd = I2C_Open(channel, devAddr);
    if (fd < 0)
        return fd;

     // OS_TaskDelay(I2C_BASE_DELAY);
    usleep(I2C_BASE_DELAY_MS * 1000);

    int32 count = read(fd, data, length);
    if (count == length)
    {
        I2C_FileClose(fd, channel);
        return count;
    }

    I2C_FileClose(fd, channel);
    I2C_ErrorCount_Read++;
    I2C_Log("Read failed (no retry). count=%d expected=%d errno=%d", count, length, errno);
    return I2C_ERR_READ;
}

int32 I2C_WriteReadBytes(uint8 channel, uint8 devAddr, uint8 cmd, uint8 wLength, void *wData, uint8 rLength,
                         void *rData, uint8 delay)
{
    /* fast path: delay==0 이고 어댑터가 I2C_RDWR 지원 -> 단일 시도 */
    if (delay == 0 && I2C_has_rdwr[channel])
    {
        int32 fd = I2C_Open(channel, devAddr);
        if (fd < 0)
            return fd;

        uint8 wbuf[wLength + 1];
        wbuf[0] = cmd;
        if (wLength > 0 && wData)
            memcpy(&wbuf[1], wData, wLength);

        struct i2c_msg             msgs[2] = {{.addr = devAddr, .flags = 0, .len = (uint16)(wLength + 1), .buf = wbuf},
                                              {.addr = devAddr, .flags = I2C_M_RD, .len = (uint16)rLength, .buf = rData}};
        struct i2c_rdwr_ioctl_data rdwr    = {.msgs = msgs, .nmsgs = 2};

        int ret = ioctl(fd, I2C_RDWR, &rdwr);
        I2C_FileClose(fd, channel);

        if (ret == 2)
            return rLength;

        /* 타임아웃/버스오류면 RDWR 비활성화 후 레거시 단일 시도로 폴백 */
        if (errno == ETIMEDOUT || errno == EIO)
        {
            I2C_has_rdwr[channel] = 0;
            I2C_Log("I2C_RDWR disabled on ch%d due to errno=%d; fallback to STOP+delay (single try)", channel, errno);
        }
        else
        {
            I2C_ErrorCount_Write++;
            I2C_ErrorCount_Read++;
            I2C_Log("I2C_RDWR failed once (ret=%d, errno=%d)", ret, errno);
            /* 이후 레거시 경로 시도 */
        }
    }

    /* legacy path: STOP 사이에 delay 허용, 단일 시도 */
    uint8 effective_delay = delay ? delay : I2C_BASE_DELAY_MS; /* 기본 20ms 지연 */

    int32 writeResult = I2C_WriteBytes(channel, devAddr, cmd, wLength, wData);
    if (writeResult != I2C_SUCCESS)
    {
        I2C_Log("WriteRead (legacy) write failed (no retry)");
        I2C_ErrorCount_Write++;
        I2C_ErrorCount_Read++;
        return I2C_ERR_WRITE_READ;
    }

    if (effective_delay)
        usleep(effective_delay * 1000);  /* ms -> us 변환 */

    int32 readResult = I2C_ReadBytes(channel, devAddr, rLength, rData);
    if (readResult == rLength)
        return readResult;

    I2C_Log("WriteRead (legacy) read failed (no retry)");
    I2C_ErrorCount_Write++;
    I2C_ErrorCount_Read++;
    return I2C_ERR_WRITE_READ;
}
