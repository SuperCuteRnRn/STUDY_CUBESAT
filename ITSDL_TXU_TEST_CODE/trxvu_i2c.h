/* ============================================================================
 * trxvu_i2c.h — Linux i2c-dev wrapper for ITSDL_TRXVU standalone test tool
 * (PLAN_TRX_HW_TEST_STANDALONE_20260521.md, 2026-05-21)
 *
 * 외부 프레임워크 무의존. 표준 Linux i2c-dev ioctl 만 사용.
 * ========================================================================== */
#ifndef TRXVU_I2C_H
#define TRXVU_I2C_H

#include <stddef.h>
#include <stdint.h>

/* === TRXVU slave addresses === */
#define TRXVU_RCV_ADDR              0x60U   /* Receiver section */
#define TRXVU_TRS_ADDR              0x61U   /* Transmitter section */

/* === Default device path on satellite OBC === */
#define TRXVU_I2C_DEFAULT_DEVICE    "/dev/i2c-1"

/* === I/O timeout (ms) === */
#define TRXVU_I2C_TIMEOUT_MS_DEFAULT  1000U

/**
 * @brief  TRXVU I²C context — open한 device fd + 메타 정보.
 *         단일 instance 가정 (CLI 환경).
 */
typedef struct
{
    int      fd;                 /* /dev/i2c-N file descriptor; -1 = closed */
    char     device_path[64];    /* 마지막 open 된 path (debug 용) */
    uint32_t timeout_ms;         /* I2C_TIMEOUT ioctl 값 (ms → ticks 자동 변환) */
} trxvu_ctx_t;

/**
 * @brief  trxvu_ctx_t 0 초기화 + fd = -1.
 *         반드시 trxvu_open 전에 호출.
 */
void trxvu_ctx_init(trxvu_ctx_t *ctx);

/**
 * @brief  /dev/i2c-N device open + I2C_TIMEOUT ioctl 설정.
 * @param  ctx          미초기화 또는 closed 상태의 context
 * @param  device_path  예: "/dev/i2c-1". NULL 이면 TRXVU_I2C_DEFAULT_DEVICE 사용
 * @return 0 on success, -1 on error (errno set)
 */
int trxvu_open(trxvu_ctx_t *ctx, const char *device_path);

/**
 * @brief  Device close. fd = -1 로 reset.
 * @return 0 on success, -1 on error
 */
int trxvu_close(trxvu_ctx_t *ctx);

/**
 * @brief  I²C write: [cmd_byte | data...] 를 slave_addr 로 전송.
 *
 * @param  ctx          open 된 context
 * @param  slave_addr   TRXVU_RCV_ADDR or TRXVU_TRS_ADDR
 * @param  cmd          명령 byte (raw CC)
 * @param  data         payload (data == NULL && len == 0 허용)
 * @param  len          payload 길이 (max 254)
 * @return 0 on success, -1 on error (errno set, perror 권장)
 */
int trxvu_write(trxvu_ctx_t *ctx, uint8_t slave_addr, uint8_t cmd,
                const uint8_t *data, size_t len);

/**
 * @brief  I²C write + read combined transaction.
 *         [cmd_byte | wdata...] 를 쓰고, repeated start 없이 약간의 settle delay
 *         후 별도 read transaction 수행 (보드 컨벤션과 일치).
 *
 * @param  ctx          open 된 context
 * @param  slave_addr   slave
 * @param  cmd          명령 byte
 * @param  wdata        write payload (NULL/0 가능)
 * @param  wlen         write payload 길이
 * @param  rdata        read 버퍼 (NULL 금지)
 * @param  rlen         read 길이
 * @return 읽은 byte 수 (>=0), -1 on error
 */
int trxvu_write_read(trxvu_ctx_t *ctx, uint8_t slave_addr, uint8_t cmd,
                     const uint8_t *wdata, size_t wlen,
                     uint8_t *rdata, size_t rlen);

/**
 * @brief  Hex dump 헬퍼 — CLI 결과 확인용.
 */
void trxvu_hex_dump(const char *label, const uint8_t *buf, size_t len);

/**
 * @brief  지정 milliseconds 만큼 sleep (usleep 기반).
 *         보드 settle delay 용.
 */
void trxvu_sleep_ms(uint32_t ms);

#endif /* TRXVU_I2C_H */
