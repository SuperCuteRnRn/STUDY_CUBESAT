/******************************************************************************
 * eps_beacon.c — EPSBAT 전체 텔레메트리 1초 주기 AX.25 송출 예제
 *
 * 공개 API 레퍼런스만 사용: epsbat.h(EPS_*) + txu.h(TXU_*).
 *   To  (지상국) : ITSDGS-0    From (위성) : ITSSAT-0
 *   콜사인은 TXU_TrsSendFrameOverride 로 프레임마다 지정(영구 변경 없음).
 *
 * (!) 경고: TXU_TrsSendFrameOverride 는 실제 RF 를 송출한다(HIGH RISK).
 *     운용 규제·주파수 허가·안테나 정합을 확인한 환경에서만 실행하라.
 *     검증 단계에서는 TX_ENABLE 을 0 으로 빌드하여 dry-run(출력만) 권장.
 *
 * 빌드: Makefile 참조 (libepsbat + libtxu 동시 링크). Linux/ARM 타겟 전용.
 * MISRA-C:2012 준수. 이탈(Deviation) 항목은 사용부 주석 참조.
 ******************************************************************************/
/* POSIX API(clock_gettime/nanosleep/sigaction) 노출 — Linux 유저공간 예제 */
#define _POSIX_C_SOURCE 200112L

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#include "epsbat.h"
#include "txu.h"

/* ── 설정 상수 ─────────────────────────────────────────────────────────── */
#define TX_ENABLE            (1)         /* 0 = dry-run(송출 없이 출력만) */
#define TLM_PERIOD_SEC       (1L)        /* 목표 송출 주기(초) */
#define NS_PER_SEC           (1000000000L)
#define FRAME_MAX            (200U)      /* 프레임당 최대 payload byte */
#define TLM_BUF_SIZE         (512U)      /* 전체 텔레메트리 문자열 버퍼 */
#define EPS_ERR_U16          (0xFFFFU)   /* EPS 조회 오류 sentinel */
#define EPS_VER_OK           (0x0001U)   /* 정상 펌웨어 버전 */
#define CS_LEN               (7U)        /* == TXU_CALLSIGN_LEN */
#define PDM_COUNT            (8U)        /* 지원 PDM 채널 수 */
#define TEMP_COUNT           (2U)        /* 보드 온도 센서 수 */
#define VSW1_CODE            (0xE410U)   /* PDM 채널1 전압 코드 */
#define VSW_CH_STEP          (16U)       /* 채널당 코드 증가폭 */
#define VSW_I_OFFSET         (4U)        /* 전류 코드 = 전압 코드 + 4 */
#define KEY_BUF_LEN          (8U)        /* "P9V" 등 키 버퍼 길이 */

/* 온도 변환: 'C = TEMP_SCALE*count - TEMP_OFFSET */
#define TEMP_SCALE           (0.372434)
#define TEMP_OFFSET          (273.15)

/* ── 콜사인 (6 ASCII + 1 SSID) ─────────────────────────────────────────── */
static const uint8_t GS_CALLSIGN[CS_LEN]  = { 'I','T','S','D','G','S', 0U }; /* To  : ITSDGS-0 */
static const uint8_t SAT_CALLSIGN[CS_LEN] = { 'I','T','S','S','A','T', 0U }; /* From: ITSSAT-0 */

/* ── 버스 텔레메트리 정의(코드/라벨/스케일) ───────────────────────────── */
typedef struct
{
    uint16_t    code;
    const char *label;
    double      scale;
} bus_item_t;

static const bus_item_t BUS_ITEMS[] =
{
    { 0xE220U, "VB",  0.008978   },  /* BAT  전압 V */
    { 0xE224U, "IB",  0.00681989 },  /* BAT  전류 A */
    { 0xE230U, "V12", 0.01349    },  /* 12V  전압 V */
    { 0xE234U, "I12", 0.00206663 },  /* 12V  전류 A */
    { 0xE210U, "V5",  0.005865   },  /* 5V   전압 V */
    { 0xE214U, "I5",  0.00681989 },  /* 5V   전류 A */
    { 0xE200U, "V3",  0.004311   },  /* 3.3V 전압 V */
    { 0xE204U, "I3",  0.00681989 }   /* 3.3V 전류 A */
};
#define BUS_COUNT (sizeof(BUS_ITEMS) / sizeof(BUS_ITEMS[0]))

/* 보드 온도 코드/라벨 */
static const uint16_t TEMP_CODE[TEMP_COUNT]   = { 0xE308U, 0xE388U };
static const char    *TEMP_LABEL[TEMP_COUNT]  = { "T1", "T2" };

/* PDM 채널 번호 / 전압·전류 스케일(레일별, API 레퍼런스 Rev1.2 4절) */
static const uint8_t PDM_CH[PDM_COUNT]     = { 1U, 2U, 3U, 4U, 5U, 6U, 8U, 9U };
static const double  PDM_VSCALE[PDM_COUNT] = { 0.01349, 0.01349, 0.008993, 0.008993,
                                               0.005865, 0.005865, 0.004311, 0.004311 };
static const double  PDM_ISCALE[PDM_COUNT] = { 0.0013283, 0.0013283, 0.0062395, 0.0062395,
                                               0.0013288, 0.0013288, 0.0013284, 0.0013284 };

/* ── 안전 종료 플래그(SIGINT) ──────────────────────────────────────────── */
static volatile sig_atomic_t g_run = 1;

static void on_sigint(int sig)
{
    (void)sig;
    g_run = 0;
}

/* ── 스케일 물리량 append (경계 안전) ──────────────────────────────────── */
/* MISRA-C:2012 Rule 21.6 deviation: Linux 유저공간 프레임 조립용 snprintf */
static size_t buf_append(char *buf, size_t size, size_t used, const char *fmt,
                         const char *key, double val, uint8_t ok)
{
    size_t result = used;
    if (used < size)
    {
        int n = 0;
        if (ok != 0U)
        {
            n = snprintf(&buf[used], size - used, fmt, key, val);
        }
        else
        {
            n = snprintf(&buf[used], size - used, "%s=ERR ", key);
        }
        if (n > 0)
        {
            result = used + (size_t)n;
            if (result > size)
            {
                result = size;
            }
        }
    }
    return result;
}

/* ── 정수(카운터/상태) append ──────────────────────────────────────────── */
static size_t buf_append_u(char *buf, size_t size, size_t used, const char *key,
                           uint16_t val)
{
    size_t result = used;
    if (used < size)
    {
        /* MISRA-C:2012 Rule 21.6 deviation: 프레임 조립용 snprintf */
        int n = snprintf(&buf[used], size - used, "%s=%u ", key, (unsigned int)val);
        if (n > 0)
        {
            result = used + (size_t)n;
            if (result > size)
            {
                result = size;
            }
        }
    }
    return result;
}

/* ── 전체 텔레메트리 조립 → buf, 반환: 사용 길이 ──────────────────────── */
static size_t build_telemetry(char *buf, size_t size, uint32_t seq)
{
    size_t   used = 0U;
    uint16_t i    = 0U;

    /* 헤더 + 상태워드 */
    {
        /* MISRA-C:2012 Rule 21.6 deviation: 프레임 조립용 snprintf */
        int n = snprintf(buf, size, "ITSSAT TLM SEQ=%lu ST=0x%04X ",
                         (unsigned long)seq, (unsigned int)EPS_GetBoardStatus());
        if (n > 0)
        {
            used = (size_t)n;
            if (used > size)
            {
                used = size;
            }
        }
    }

    /* 전원 버스 8종 */
    for (i = 0U; i < (uint16_t)BUS_COUNT; i++)
    {
        uint8_t  ok  = 0U;
        uint16_t raw = EPS_GetTelemetry(BUS_ITEMS[i].code);
        double   phy = 0.0;
        if (raw != EPS_ERR_U16)
        {
            ok  = 1U;
            phy = (double)raw * BUS_ITEMS[i].scale;
        }
        used = buf_append(buf, size, used, "%s=%.2f ", BUS_ITEMS[i].label, phy, ok);
    }

    /* 보드 온도 2종 */
    for (i = 0U; i < (uint16_t)TEMP_COUNT; i++)
    {
        uint8_t  ok  = 0U;
        uint16_t raw = EPS_GetTelemetry(TEMP_CODE[i]);
        double   phy = 0.0;
        if (raw != EPS_ERR_U16)
        {
            ok  = 1U;
            phy = (TEMP_SCALE * (double)raw) - TEMP_OFFSET;
        }
        used = buf_append(buf, size, used, "%s=%.1f ", TEMP_LABEL[i], phy, ok);
    }

    /* PDM 채널 텔레메트리(1~6,8,9) 각 V/I */
    for (i = 0U; i < (uint16_t)PDM_COUNT; i++)
    {
        char     klv[KEY_BUF_LEN] = { 0 };
        char     kli[KEY_BUF_LEN] = { 0 };
        uint16_t vcode = (uint16_t)(VSW1_CODE
                       + ((uint16_t)VSW_CH_STEP * (uint16_t)(PDM_CH[i] - 1U)));
        uint16_t icode = (uint16_t)(vcode + VSW_I_OFFSET);
        uint16_t vraw  = EPS_GetTelemetry(vcode);
        uint16_t iraw  = EPS_GetTelemetry(icode);
        uint8_t  vok   = (vraw != EPS_ERR_U16) ? 1U : 0U;
        uint8_t  iok   = (iraw != EPS_ERR_U16) ? 1U : 0U;
        double   vphy  = (vok != 0U) ? ((double)vraw * PDM_VSCALE[i]) : 0.0;
        double   iphy  = (iok != 0U) ? ((double)iraw * PDM_ISCALE[i]) : 0.0;

        /* MISRA-C:2012 Rule 21.6 deviation: 키 문자열 생성용 snprintf */
        (void)snprintf(klv, sizeof(klv), "P%uV", (unsigned int)PDM_CH[i]);
        (void)snprintf(kli, sizeof(kli), "P%uI", (unsigned int)PDM_CH[i]);
        used = buf_append(buf, size, used, "%s=%.2f ", klv, vphy, vok);
        used = buf_append(buf, size, used, "%s=%.3f ", kli, iphy, iok);
    }

    /* 리셋 카운터 4종 */
    used = buf_append_u(buf, size, used, "BOR", EPS_GetNumberOfBrownOutReset());
    used = buf_append_u(buf, size, used, "AR",  EPS_GetNumberOfAutomaticReset());
    used = buf_append_u(buf, size, used, "MR",  EPS_GetNumberOfManuelReset());
    used = buf_append_u(buf, size, used, "CWR", EPS_GetNumberOfCommWatchdogReset());

    return used;
}

/* ── 버퍼를 프레임 청크로 분할 송출 ────────────────────────────────────── */
static int send_telemetry(const char *buf, size_t total)
{
    int    status = 0;
    size_t offset = 0U;

    while ((offset < total) && (status == 0))
    {
        size_t  remain = total - offset;
        uint8_t chunk  = (remain > (size_t)FRAME_MAX)
                       ? (uint8_t)FRAME_MAX : (uint8_t)remain;
#if (TX_ENABLE == 1)
        uint8_t rem = 0U;
        int rc = TXU_TrsSendFrameOverride(GS_CALLSIGN, SAT_CALLSIGN,
                                          (const uint8_t *)&buf[offset], chunk, &rem);
        if (rc != 0)
        {
            status = -1;
        }
#else
        /* MISRA-C:2012 Rule 21.6 deviation: dry-run 진단 출력 */
        (void)printf("[DRY-RUN TX %u B] %.*s\n",
                     (unsigned int)chunk, (int)chunk, &buf[offset]);
#endif
        offset += (size_t)chunk;
    }
    return status;
}

/* ── 주기 대기(1초 − 경과시간) ─────────────────────────────────────────── */
static void sleep_remainder(const struct timespec *start)
{
    struct timespec now = { 0, 0 };
    (void)clock_gettime(CLOCK_MONOTONIC, &now);
    {
        long sec        = (long)now.tv_sec - (long)start->tv_sec;
        long nsec       = (long)now.tv_nsec - (long)start->tv_nsec;
        long elapsed_ns = (sec * NS_PER_SEC) + nsec;
        long target_ns  = TLM_PERIOD_SEC * NS_PER_SEC;
        if (elapsed_ns < target_ns)
        {
            struct timespec req    = { 0, 0 };
            long            rem_ns = target_ns - elapsed_ns;
            req.tv_sec  = (time_t)(rem_ns / NS_PER_SEC);
            req.tv_nsec = rem_ns % NS_PER_SEC;
            (void)nanosleep(&req, NULL);
        }
    }
}

/* ── main ──────────────────────────────────────────────────────────────── */
int main(void)
{
    int              exit_code = 0;
    struct sigaction sa;

    (void)memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_sigint;
    (void)sigaction(SIGINT, &sa, NULL);

    (void)printf("=== EPSBAT TLM beacon  To=ITSDGS-0  From=ITSSAT-0 ===\n");
#if (TX_ENABLE == 1)
    (void)printf("(!) RF 송출 모드. Ctrl-C 로 중단.\n");
#else
    (void)printf("(dry-run: 송출 없이 프레임만 출력) Ctrl-C 로 중단.\n");
#endif

    if (EPS_GetVersion() != EPS_VER_OK)
    {
        (void)printf("EPS board not found (version mismatch).\n");
        exit_code = 1;
    }
    else
    {
        uint32_t seq = 0U;
        while (g_run != 0)
        {
            struct timespec start = { 0, 0 };
            char            frame[TLM_BUF_SIZE];
            size_t          len = 0U;

            (void)memset(frame, 0, sizeof(frame));
            (void)clock_gettime(CLOCK_MONOTONIC, &start);

            len = build_telemetry(frame, sizeof(frame), seq);
            if (send_telemetry(frame, len) != 0)
            {
                (void)printf("TX error (seq=%lu)\n", (unsigned long)seq);
            }
            seq++;
            sleep_remainder(&start);
        }
        (void)printf("\nstopped by user.\n");
    }

    TXU_Close();
    return exit_code;
}
