/******************************************************************************
 * rx_img_tx.c — 프리셋10 수신 시 촬영·이미지 조각 전송 (ITSDL_TRXVU)
 *
 * ★ I2C 계층: 검증된 벤더 라이브러리 사용 (Manufacturing_data 참조).
 *   - i2c_lib.c        : 트랜잭션마다 open→settle→write/read→close + 재시도.
 *   - trx_rcv_function.c: 0x22 Get Frame 응답의 6B 헤더(size/doppler/rssi)를
 *                         정확히 벗겨 순수 payload(frame_contents)만 제공.
 *   (libtxu 는 슬레이브 선택 직후 즉시 write 하여 프레임 읽기에서 EIO 가 났음.)
 *
 * 동작: 지상국이 프리셋 10(ASCII "10")을 송신하면 라즈베리파이가
 *   rpicam-still --nopreview --autofocus-mode auto --width 320 --height 320 -o img1.jpg
 * 로 촬영한 뒤 JPEG 를 조각내어 TRS(0x61)로 다운링크한다.
 *
 * 온에어 포맷(TRS send-frame 0x10 payload):
 *   'IMG'(3) + img_id(1) + seq(2, BE) + total(2, BE) + chunk(<=200)
 *
 * 빌드: make    (i2c_lib.c + trx_rcv_function.c 함께 컴파일, -lpthread)
 * 실행: sudo ./rx_img_tx
 *
 * MISRA-C:2012: 본 파일은 단일 출구/초기화/캐스트/동적할당 없음 준수. 승인 예외:
 *   Rule 21.8: system() 대신 fork/execvp/waitpid.
 *   Rule 21.6: 로그 printf, 파일은 POSIX open/read.
 *   Rule 7.4 : execvp argv 는 static char[].
 *   (벤더 파일 i2c_lib.c/trx_rcv_function.c 는 원본 그대로 사용 — 별도 준수대상 아님.)
 ******************************************************************************/
#define _DEFAULT_SOURCE 1

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <stdio.h>   /* MISRA-C:2012 Rule 21.6 deviation: 로그 출력용 printf */

#include "i2c_lib.h"
#include "trx_rcv_function.h"

/* ── I2C / TRXVU 주소·명령 ─────────────────────────────────────────────── */
#define I2C_CH            1U          /* i2c_lib channel 1 = /dev/i2c-1 */
#define RCV_ADDR          0x60U       /* 수신부 */
#define TRS_ADDR          0x61U       /* 송신부 */
#define CC_TRS_SEND_FRAME 0x10U
#define CC_TRS_SET_IDLE   0x24U
#define TX_RD_DELAY_MS    5U          /* send-frame write→read 지연 */

/* ── 이미지 조각 포맷 상수 ─────────────────────────────────────────────── */
#define IMG_PATH          "img.rgb"
#define IMG_W             64U          /* 전송 이미지 폭(px) */
#define IMG_H             64U          /* 전송 이미지 높이(px) */
#define IMG_MAGIC0        ((uint8_t)'I')
#define IMG_MAGIC1        ((uint8_t)'M')
#define IMG_MAGIC2        ((uint8_t)'G')
#define IMG_HDR_LEN       8U          /* magic(3)+id(1)+seq(2)+total(2) */
#define IMG_CHUNK_MAX     200U        /* 프레임당 JPEG 조각 최대 */
#define IMG_FRAME_MAX     (IMG_HDR_LEN + IMG_CHUNK_MAX)   /* 208 <= 235 */
#define IMG_BUF_MAX       262144U     /* 촬영 JPEG 최대(256KB) static */

/* ── 타이밍/기타 ───────────────────────────────────────────────────────── */
#define RX_POLL_NS        200000000L
#define TX_GAP_NS         50000000L
#define TX_RETRY_MAX      20U
#define IMG_TX_REPEAT     3U          /* 손실 대비 이미지 반복 전송 횟수 */
#define TRS_TX_BUF_FULL   0xFFU        /* 0x10 응답: 버퍼 포화(프레임 미수록) */
#define TX_FULL_WAIT_NS   200000000L   /* 버퍼 풀 시 슬롯 대기(0.2s) */
#define NS_PER_SEC        1000000000L
#define BYTE_MASK         0xFFU
#define BYTE_SHIFT        8U
#define AX25_CTRL_UI      0x03U
#define AX25_PID_NOL3     0xF0U
#define IDLE_ON           1U
#define IDLE_OFF          0U
#define RPICAM_FAIL_EXIT  127

static volatile sig_atomic_t g_running = 1;

static void on_signal(int sig)
{
    (void)sig;
    g_running = 0;
}

static void sleep_ns(long ns)
{
    struct timespec ts;

    ts.tv_sec  = ns / NS_PER_SEC;
    ts.tv_nsec = ns % NS_PER_SEC;
    (void)nanosleep(&ts, NULL);
}

/* ── rpicam-still 촬영 (fork/execvp). 성공 0 / 실패 -1 ─────────────────── *
 * MISRA-C:2012 Rule 21.8 deviation: system() 대신 fork/execvp/waitpid.       */
static int capture_image(void)
{
    int   result = -1;
    pid_t pid    = fork();

    if (pid < 0)
    {
        printf("[ERR] fork 실패: %s\n", strerror(errno));
    }
    else if (pid == 0)
    {
        /* Rule 7.4 회피: 문자열 리터럴 대신 static char[] */
        static char a_cmd[] = "rpicam-still";
        static char a_np[]  = "--nopreview";
        static char a_af[]  = "--autofocus-mode";
        static char a_afv[] = "auto";
        static char a_w[]   = "--width";
        static char a_wv[]  = "64";
        static char a_h[]   = "--height";
        static char a_hv[]  = "64";
        static char a_enc[] = "--encoding";
        static char a_encv[] = "rgb";
        static char a_o[]   = "-o";
        static char a_ov[]  = IMG_PATH;
        char *const argv[]  = {
            a_cmd, a_np, a_af, a_afv, a_w, a_wv, a_h, a_hv,
            a_enc, a_encv, a_o, a_ov, NULL
        };

        (void)execvp(a_cmd, argv);
        _exit(RPICAM_FAIL_EXIT);
    }
    else
    {
        int   status = 0;
        pid_t w      = waitpid(pid, &status, 0);

        if ((w == pid) && (WIFEXITED(status) != 0) && (WEXITSTATUS(status) == 0))
        {
            result = 0;
        }
        else
        {
            printf("[ERR] rpicam-still 실패 (status=%d)\n", status);
        }
    }
    return result;
}

/* ── 파일을 버퍼에 읽음 (POSIX). 읽은 byte 수 / -1 ─────────────────────── *
 * MISRA-C:2012 Rule 21.6 회피: <stdio.h> 대신 POSIX open/read/close.        */
static long read_file(const char *path, uint8_t *buf, size_t max)
{
    long total = -1;
    int  fd    = open(path, O_RDONLY);

    if (fd < 0)
    {
        printf("[ERR] open %s 실패: %s\n", path, strerror(errno));
    }
    else
    {
        size_t off  = 0U;
        int    done = 0;

        while ((done == 0) && (off < max))
        {
            ssize_t n = read(fd, &buf[off], max - off);

            if (n < 0)
            {
                if (errno != EINTR)
                {
                    done = 2;
                }
            }
            else if (n == 0)
            {
                done = 1;
            }
            else
            {
                off += (size_t)n;
            }
        }
        (void)close(fd);

        if (done == 1)
        {
            total = (long)off;
        }
        else if (off >= max)
        {
            printf("[ERR] 이미지가 버퍼(%uB)보다 큼\n", (unsigned int)max);
        }
        else
        {
            printf("[ERR] 파일 읽기 오류: %s\n", strerror(errno));
        }
    }
    return total;
}

/* ── 수신 payload 가 프리셋10("10") 트리거인지 판정. 1/0 ───────────────── */
static int frame_is_trigger(const uint8_t *buf, size_t len)
{
    int            is_trig = 0;
    const uint8_t *info    = buf;
    size_t         ilen    = len;
    size_t         i       = 0U;

    /* AX.25 UI 헤더(... 0x03 0xF0 info)가 있으면 info 부분만 취함 */
    for (i = 0U; (i + 1U) < len; i++)
    {
        if ((buf[i] == AX25_CTRL_UI) && (buf[i + 1U] == AX25_PID_NOL3))
        {
            info = &buf[i + 2U];
            ilen = len - (i + 2U);
            break;
        }
    }
    while ((ilen > 0U) &&
           ((info[0] == (uint8_t)' ') || (info[0] == (uint8_t)'\r') ||
            (info[0] == (uint8_t)'\n')))
    {
        info = &info[1];
        ilen--;
    }
    while ((ilen > 0U) &&
           ((info[ilen - 1U] == (uint8_t)' ') || (info[ilen - 1U] == (uint8_t)'\r') ||
            (info[ilen - 1U] == (uint8_t)'\n')))
    {
        ilen--;
    }
    if ((ilen == 2U) && (info[0] == (uint8_t)'1') && (info[1] == (uint8_t)'0'))
    {
        is_trig = 1;
    }
    return is_trig;
}

/* ── TRS idle 모드 설정 (0x24). 성공 0 / 실패 -1 ──────────────────────── */
static int trs_set_idle(uint8_t enable)
{
    int     result = -1;
    uint8_t e      = enable;

    if (I2C_WriteBytes(I2C_CH, TRS_ADDR, CC_TRS_SET_IDLE, 1U, &e) == I2C_SUCCESS)
    {
        result = 0;
    }
    return result;
}

/* ── 한 조각 프레임 송신(0x10, 재시도). 성공 0 / 실패 -1 ──────────────── */
static int send_one(uint8_t *frame, uint8_t flen)
{
    int     result = -1;
    uint8_t tries  = 0U;

    for (tries = 0U; (tries < TX_RETRY_MAX) && (result != 0); tries++)
    {
        uint8_t rem = 0U;

        /* write [0x10 | frame], read 1 byte(남은 TX 슬롯 수) */
        if (I2C_WriteReadBytes(I2C_CH, TRS_ADDR, CC_TRS_SEND_FRAME,
                               flen, frame, 1U, &rem, TX_RD_DELAY_MS) == 1)
        {
            if (rem != TRS_TX_BUF_FULL)
            {
                result = 0;               /* 슬롯에 실제 수록됨 → 성공 */
            }
            else
            {
                /* 버퍼 풀 → 슬롯이 빌 때까지 대기 후 동일 프레임 재시도 */
                sleep_ns(TX_FULL_WAIT_NS);
            }
        }
        else
        {
            sleep_ns(TX_GAP_NS);          /* I2C 실패 → 짧게 대기 후 재시도 */
        }
    }
    return result;
}

/* ── raw RGB 를 오프셋 스트립으로 전송. idle ON → 전송 → idle OFF.
      각 프레임이 위치 독립적이라 지상국이 도착 즉시 제자리에 표시. 0/-1 ──── */
static int send_image(const uint8_t *raw, size_t rlen, uint8_t img_id)
{
    int     result = 0;
    size_t  off    = 0U;
    uint8_t frame[IMG_FRAME_MAX];

    if (trs_set_idle(IDLE_ON) != 0)
    {
        printf("[WARN] SetIdle(ON) 실패 — 계속 진행\n");
    }

    while ((off < rlen) && (result == 0))
    {
        size_t  clen = rlen - off;
        uint8_t flen = 0U;

        if (clen > IMG_CHUNK_MAX)
        {
            clen = IMG_CHUNK_MAX;
        }
        frame[0] = IMG_MAGIC0;
        frame[1] = IMG_MAGIC1;
        frame[2] = IMG_MAGIC2;
        frame[3] = img_id;
        frame[4] = (uint8_t)IMG_W;
        frame[5] = (uint8_t)IMG_H;
        frame[6] = (uint8_t)((off >> BYTE_SHIFT) & BYTE_MASK);
        frame[7] = (uint8_t)(off & BYTE_MASK);
        (void)memcpy(&frame[IMG_HDR_LEN], &raw[off], clen);
        flen = (uint8_t)(IMG_HDR_LEN + clen);

        if (send_one(frame, flen) != 0)
        {
            printf("[ERR] off %u 송신 실패\n", (unsigned int)off);
            result = -1;
        }
        else
        {
            off += clen;
            sleep_ns(TX_GAP_NS);
        }
    }

    if (trs_set_idle(IDLE_OFF) != 0)
    {
        printf("[WARN] SetIdle(OFF) 실패\n");
    }
    if (result == 0)
    {
        printf("[OK] 이미지 id=%u 전송 완료 (raw %ux%u, %u B)\n",
               (unsigned int)img_id, (unsigned int)IMG_W,
               (unsigned int)IMG_H, (unsigned int)rlen);
    }
    return result;
}

/* ── 트리거 1건 처리: 촬영 → 읽기 → 전송. 전송 성공 시 1 반환 ─────────── */
static int do_capture_and_send(uint8_t *jpeg_buf, uint8_t img_id)
{
    int sent_ok = 0;

    if (capture_image() != 0)
    {
        printf("[ERR] 촬영 실패 — 전송 생략\n");
    }
    else
    {
        long n = read_file(IMG_PATH, jpeg_buf, IMG_BUF_MAX);

        if (n > 0)
        {
            uint8_t pass = 0U;

            /* 손실 대비: 같은 img_id 로 반복 전송(지상국이 seq별로 구멍 채움) */
            for (pass = 0U; (pass < IMG_TX_REPEAT) && (g_running != 0); pass++)
            {
                printf("[TX] id=%u 반복 %u/%u\n",
                       (unsigned int)img_id,
                       (unsigned int)(pass + 1U),
                       (unsigned int)IMG_TX_REPEAT);
                if (send_image(jpeg_buf, (size_t)n, img_id) == 0)
                {
                    sent_ok = 1;
                }
            }
        }
        else
        {
            printf("[ERR] 이미지 읽기 실패\n");
        }
    }
    return sent_ok;
}

int main(void)
{
    int              rc     = 0;
    uint8_t          img_id = 1U;
    static uint8_t   jpeg_buf[IMG_BUF_MAX];
    struct sigaction sa;

    (void)memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_signal;
    (void)sigaction(SIGINT, &sa, NULL);
    (void)sigaction(SIGTERM, &sa, NULL);

    if (I2C_LibInit() != 0)
    {
        printf("[ERR] I2C 초기화 실패\n");
        rc = 1;
    }
    else
    {
        printf("[RDY] 프리셋10 수신 대기 (Ctrl-C 종료)\n");

        while (g_running != 0)
        {
            uint16_t cnt = 0U;

            if (TRX_RCV_Get_Frame_num((uint8_t)RCV_ADDR, &cnt) < 0)
            {
                sleep_ns(RX_POLL_NS);
            }
            else if (cnt == 0U)
            {
                sleep_ns(RX_POLL_NS);
            }
            else
            {
                TRX_RCS_RES_Frame_buf fb;
                size_t                plen = 0U;
                int                   trig = 0;

                (void)memset(&fb, 0, sizeof(fb));
                TRX_RCV_Get_Frame((uint8_t)RCV_ADDR, &fb);
                TRX_RCV_Remove_Frame((uint8_t)RCV_ADDR);

                plen = (size_t)fb.frame_size;
                if (plen > (size_t)TRX_FRAME_SIZE_UP)
                {
                    plen = (size_t)TRX_FRAME_SIZE_UP;
                }
                trig = frame_is_trigger(fb.frame_contents, plen);
                printf("[RX] frame %uB%s\n", (unsigned int)plen,
                       (trig != 0) ? " = 프리셋10 (촬영)" : "");

                if (trig != 0)
                {
                    if (do_capture_and_send(jpeg_buf, img_id) != 0)
                    {
                        img_id++;
                    }
                }
            }
        }
        printf("\n[BYE] 종료\n");
    }
    return rc;
}
