/* ============================================================================
 * trxvu_i2c.c — Linux i2c-dev wrapper implementation
 * (PLAN_TRX_HW_TEST_STANDALONE_20260521.md, 2026-05-21)
 *
 * NPR 7150.2D Class E (Ground SW). MISRA Rule 15.5 단일 출구 가능한 한 적용.
 * ========================================================================== */
#include "trxvu_i2c.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>

/* settle delay between write and read transaction (us).
 * ITSDL_TRXVU response timing: ICD-0001 §7 권장 minimum 5 ms.
 * cFS i2c_lib 의 I2C_PRE_IO_DELAY = 1 ms 와 부합. */
#define TRXVU_WR_RD_SETTLE_US   1000U   /* 1 ms */

/* Linux I2C_TIMEOUT ioctl unit = 10ms tick — ms → ticks 변환 (ceil) */
#define MS_TO_I2C_TICKS(ms)   (((ms) + 9U) / 10U)

void trxvu_ctx_init(trxvu_ctx_t *ctx)
{
    if (ctx != NULL)
    {
        ctx->fd          = -1;
        ctx->device_path[0] = '\0';
        ctx->timeout_ms  = TRXVU_I2C_TIMEOUT_MS_DEFAULT;
    }
}

int trxvu_open(trxvu_ctx_t *ctx, const char *device_path)
{
    int result = 0;
    const char *path = (device_path != NULL) ? device_path : TRXVU_I2C_DEFAULT_DEVICE;

    if (ctx == NULL)
    {
        errno = EINVAL;
        result = -1;
    }
    else if (ctx->fd >= 0)
    {
        fprintf(stderr, "trxvu_open: already open (fd=%d)\n", ctx->fd);
        errno = EBUSY;
        result = -1;
    }
    else
    {
        ctx->fd = open(path, O_RDWR);
        if (ctx->fd < 0)
        {
            perror("open(/dev/i2c-N)");
            result = -1;
        }
        else
        {
            (void)strncpy(ctx->device_path, path, sizeof(ctx->device_path) - 1U);
            ctx->device_path[sizeof(ctx->device_path) - 1U] = '\0';

            /* I2C_TIMEOUT — best-effort. 일부 커널에서 미지원, 실패해도 진행. */
            if (ctx->timeout_ms == 0U)
            {
                ctx->timeout_ms = TRXVU_I2C_TIMEOUT_MS_DEFAULT;
            }
            if (ioctl(ctx->fd, I2C_TIMEOUT, (unsigned long)MS_TO_I2C_TICKS(ctx->timeout_ms)) < 0)
            {
                fprintf(stderr, "trxvu_open: I2C_TIMEOUT ioctl warn (%s), continuing\n",
                        strerror(errno));
            }
            printf("trxvu_open: opened %s (fd=%d, timeout=%u ms)\n",
                   ctx->device_path, ctx->fd, ctx->timeout_ms);
        }
    }

    return result;
}

int trxvu_close(trxvu_ctx_t *ctx)
{
    int result = 0;

    if ((ctx != NULL) && (ctx->fd >= 0))
    {
        if (close(ctx->fd) < 0)
        {
            perror("close");
            result = -1;
        }
        ctx->fd = -1;
    }

    return result;
}

int trxvu_write(trxvu_ctx_t *ctx, uint8_t slave_addr, uint8_t cmd,
                const uint8_t *data, size_t len)
{
    int     result = 0;
    uint8_t buf[256];
    ssize_t n      = 0;

    if ((ctx == NULL) || (ctx->fd < 0))
    {
        errno = EBADF;
        result = -1;
    }
    else if (len > (sizeof(buf) - 1U))
    {
        errno = EMSGSIZE;
        result = -1;
    }
    else
    {
        /* I2C_SLAVE: 슬레이브 주소 설정 (per-fd state) */
        if (ioctl(ctx->fd, I2C_SLAVE, (unsigned long)slave_addr) < 0)
        {
            perror("ioctl(I2C_SLAVE)");
            result = -1;
        }
        else
        {
            buf[0] = cmd;
            if ((data != NULL) && (len > 0U))
            {
                (void)memcpy(&buf[1], data, len);
            }

            n = write(ctx->fd, buf, len + 1U);
            if (n < 0)
            {
                perror("write");
                result = -1;
            }
            else if ((size_t)n != (len + 1U))
            {
                fprintf(stderr, "trxvu_write: short write %zd/%zu\n", n, len + 1U);
                errno = EIO;
                result = -1;
            }
            else
            {
                result = 0;
            }
        }
    }

    return result;
}

int trxvu_write_read(trxvu_ctx_t *ctx, uint8_t slave_addr, uint8_t cmd,
                     const uint8_t *wdata, size_t wlen,
                     uint8_t *rdata, size_t rlen)
{
    int     result = -1;
    uint8_t wbuf[256];
    ssize_t n      = 0;

    if ((ctx == NULL) || (ctx->fd < 0))
    {
        errno = EBADF;
    }
    else if ((rdata == NULL) || (rlen == 0U) || (wlen > (sizeof(wbuf) - 1U)))
    {
        errno = EINVAL;
    }
    else
    {
        if (ioctl(ctx->fd, I2C_SLAVE, (unsigned long)slave_addr) < 0)
        {
            perror("ioctl(I2C_SLAVE)");
        }
        else
        {
            /* (1) write cmd + payload */
            wbuf[0] = cmd;
            if ((wdata != NULL) && (wlen > 0U))
            {
                (void)memcpy(&wbuf[1], wdata, wlen);
            }
            n = write(ctx->fd, wbuf, wlen + 1U);
            if (n < 0)
            {
                perror("write(wr-rd)");
            }
            else if ((size_t)n != (wlen + 1U))
            {
                fprintf(stderr, "trxvu_write_read: short write %zd/%zu\n", n, wlen + 1U);
                errno = EIO;
            }
            else
            {
                /* (2) settle delay (보드 컨벤션) */
                trxvu_sleep_ms(TRXVU_WR_RD_SETTLE_US / 1000U);

                /* (3) read response */
                n = read(ctx->fd, rdata, rlen);
                if (n < 0)
                {
                    perror("read");
                }
                else
                {
                    result = (int)n;
                }
            }
        }
    }

    return result;
}

void trxvu_hex_dump(const char *label, const uint8_t *buf, size_t len)
{
    const char *lbl = (label != NULL) ? label : "data";
    size_t i;

    printf("%s [%zu B]:", lbl, len);
    if (buf != NULL)
    {
        for (i = 0U; i < len; i++)
        {
            printf(" %02X", (unsigned)buf[i]);
        }
    }
    printf("\n");
}

void trxvu_sleep_ms(uint32_t ms)
{
    /* usleep argument max = 1e6 in some libc. ms 단위로 분할. */
    while (ms > 0U)
    {
        uint32_t chunk_ms = (ms > 100U) ? 100U : ms;
        (void)usleep((useconds_t)chunk_ms * 1000U);
        ms -= chunk_ms;
    }
}
