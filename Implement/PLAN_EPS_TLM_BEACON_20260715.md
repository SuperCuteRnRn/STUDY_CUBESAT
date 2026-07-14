# PLAN_EPS_TLM_BEACON_20260715

EPSBAT 보드 **전체 텔레메트리**를 **1초 주기**로 AX.25 프레임에 실어
통신보드(TXU)로 RF 송출하는 예제 코드 (C + Python).

- 지상국(To, 수신처)   콜사인: **ITSDGS**, SSID **0**
- 위성(From, 송신원)   콜사인: **ITSSAT**, SSID **0**
- **공개 API 레퍼런스만 사용** — `epsbat.h`(EPS_*) + `txu.h`(TXU_*). 데이터시트의 raw
  I2C 프로토콜/레지스터 직접 접근·내부(`trxvu_*`) 계층은 사용하지 않음.

---

## 1. 작업 목적 및 배경

- 교육용 큐브위성의 전원보드(ITSDL_EPS&BAT)에서 측정되는 모든 텔레메트리를
  주기적으로 지상국으로 내려보내는 **텔레메트리 비콘**을 예제로 구현한다.
- 두 개의 독립 라이브러리(`libepsbat`, `libtxu`)를 하나의 응용에서 함께 링크하여
  "센서 읽기(EPS) → 프레임 조립 → RF 송출(TXU)" 파이프라인을 보인다.
- 콜사인은 **영구 변경 없이** 프레임 단위로 지정한다
  (`TXU_TrsSetTo/FromCallsign` = 영구 변경(!) 이므로 사용하지 않음).
  대신 `TXU_TrsSendFrameOverride(to, from, ...)` 로 매 프레임 To/From 을 지정한다.

### 근거 API (레퍼런스에서 직접 확인)

| 용도 | 함수 | 반환/규약 |
|---|---|---|
| 펌웨어 버전 확인 | `EPS_GetVersion()` | `0x0001` 정상 |
| 보드 상태워드 | `EPS_GetBoardStatus()` | `0x0000` 정상 |
| 마지막 오류 | `EPS_GetLastError()` | 0x01 명령/0x02 데이터/0x03 채널·TLE |
| 텔레메트리 | `EPS_GetTelemetry(uint16 TLECode)` | 10-bit ADC count(0~1023), 오류 `0xFFFF` |
| 리셋 카운터 | `EPS_GetNumberOfBrownOutReset/AutomaticReset/ManuelReset/CommWatchdogReset()` | uint16, 오류 `0xFFFF` |
| RF 송출(콜사인 지정) | `TXU_TrsSendFrameOverride(to[7],from[7],frame,len,*rem)` | `0`성공/`-1`오류 **(!) HIGH: RF 송출** |
| 종료 정리 | `TXU_Close()` | void |

### 콜사인 인코딩 (레퍼런스 명시: "7 byte = 6 ASCII + 1 SSID")

flat API 가 내부에서 AX.25 인코딩을 처리하므로 호출자는 **6 ASCII + SSID 1바이트**만 전달.
6자 미만이면 공백(`0x20`) 패딩. 두 콜사인 모두 정확히 6자이므로 패딩 불필요.

```
To   (지상국) ITSDGS-0 : { 'I','T','S','D','G','S', 0x00 }
From (위성)   ITSSAT-0 : { 'I','T','S','S','A','T', 0x00 }
```

---

## 2. 작업 범위 (변경/추가 대상 파일)

신규 생성만 하며, 기존 파일은 수정하지 않는다.

```
Examples/eps_tlm_beacon/
  eps_beacon.c      C 예제 (MISRA-C:2012 준수)
  eps_beacon.py     Python(ctypes) 예제
  Makefile          C 예제 빌드 (libepsbat + libtxu 동시 링크)
  README.md         빌드/실행/안전 안내
```

라이브러리(`libepsbat.a/.so`, `libtxu.a/.so`)는 배포 규칙상 타겟(라즈베리파이/ARM)에서
`make dist` 후 생성된다. 본 예제는 헤더 include 경로와 링크 플래그만 참조한다.

---

## 3. 전송하는 "전체 텔레메트리" 목록

모든 값은 `EPS_GetTelemetry(코드)` 로 ADC count 를 읽고 스케일을 곱해 물리량으로 변환.
스케일·공식은 `epsbat.h` 및 API 레퍼런스(Rev1.2) 4·5절에서 확인. (레퍼런스 PDF 는
임베디드 폰트라 ToUnicode 로 디코딩 후 `epsbat.h` 값과 교차검증하여 확정.)

### 3.1 전원 버스 (8종)
| 코드(TLE) | 항목 | 스케일 | 단위 |
|---|---|---|---|
| `0xE220 VPCMBATV` | BAT 버스 전압 | ×0.008978 | V |
| `0xE224 IPCMBATV` | BAT 버스 전류¹ | ×0.00681989 | A |
| `0xE230 VPCM12V`  | 12V 버스 전압 | ×0.01349 | V |
| `0xE234 IPCM12V`  | 12V 버스 전류 | ×0.00206663 | A |
| `0xE210 VPCM5V`   | 5V  버스 전압 | ×0.005865 | V |
| `0xE214 IPCM5V`   | 5V  버스 전류 | ×0.00681989 | A |
| `0xE200 VPCM3V3`  | 3.3V 버스 전압 | ×0.004311 | V |
| `0xE204 IPCM3V3`  | 3.3V 버스 전류 | ×0.00681989 | A |

¹ 배터리 전류는 방전 방향 계측.

### 3.2 보드 온도 (2종) — `온도('C) = 0.372434 × count − 273.15`
| 코드 | 항목 |
|---|---|
| `0xE308 TBRD`    | 보드 온도 1 |
| `0xE388 TBRD_DB` | 보드 온도 2 |

### 3.3 PDM 채널 텔레메트리 (지원 채널 1~6, 8, 9 = 8채널, 각 V/I)
- 전압 코드 = `EPS_TLE_VSW1(0xE410) + 16×(채널−1)`, 전류 코드 = 전압 코드 + 4.
- 채널→레일 매핑 및 스케일(API 레퍼런스 4절 표, ToUnicode 디코딩 + 교차검증):

| 채널 | 레일 | 전압 스케일(V) | 전류 스케일(A) |
|---|---|---|---|
| 1,2 | 12V  | 0.01349  | 0.0013283 |
| 3,4 | BAT  | 0.008993 | 0.0062395 |
| 5,6 | 5V   | 0.005865 | 0.0013288 |
| 8,9 | 3.3V | 0.004311 | 0.0013284 |

> ⚠ PDM 채널별 스케일은 API 레퍼런스 PDF에만 존재(헤더 미포함). ToUnicode 디코딩
> 값을 헤더의 버스 스케일과 교차확인했으나, 정밀도가 중요하면 원문(렌더링) 재확인 권장.

### 3.4 상태/카운터 (부가 텔레메트리)
- `EPS_GetBoardStatus()` (상태워드), `EPS_GetLastError()`
- 리셋 카운터 4종: BrownOut(워치독)/Automatic(SW)/Manuel(핀)/CommWatchdog(전원투입)

### 3.5 AX.25 페이로드 형식 (ASCII 변환 물리값)
사람이 읽는 `KEY=VALUE` 공백 구분 텍스트. 전체 문자열이 프레임 한도(200 byte)를 넘으면
자동으로 여러 프레임으로 분할 송출(각 프레임 동일 To/From). 예:

```
ITSSAT TLM SEQ=12 ST=0x0000 VB=7.85 IB=0.34 V12=12.10 I12=0.05 V5=5.01 I5=0.20
V3=3.31 I3=0.15 T1=23.4 T2=24.1 P1V=12.09 P1I=0.02 ... P9V=3.30 P9I=0.01
BOR=0 AR=1 MR=0 CWR=2
```

---

## 4. 구현 방법 (단계별) 및 전체 코드

### 4.1 처리 흐름
1. 시작 배너 + 안전 경고 출력. `TX_ENABLE==0` 이면 dry-run(송출 없이 출력).
2. `EPS_GetVersion()==0x0001` 로 EPS 보드 존재 확인. 실패 시 종료.
3. 루프(SIGINT 로 중단):
   a. `CLOCK_MONOTONIC` 로 주기 시작시각 기록.
   b. 모든 텔레메트리 조회 → ASCII 버퍼 조립(오류값은 `ERR` 로 표기).
   c. 버퍼를 200 byte 이하 청크로 나눠 `TXU_TrsSendFrameOverride(To,From,...)` 송출.
   d. 경과시간 계산 → `1초 − 경과` 만큼 sleep(음수면 즉시 다음 주기).
4. `TXU_Close()` 정리 후 종료.

> ⏱ 주의: 레퍼런스가 "EPS 호출 한 건당 수백 ms 소요 가능"이라 명시. 전체(~30회) 조회
> 시간이 1초를 넘으면 실제 주기는 1초보다 길어지고 back-to-back 으로 동작한다.
> 1초는 **목표(최소) 주기**이며 보드 응답속도에 따라 실측 주기는 달라진다.

### 4.2 MISRA-C:2012 준수 및 이탈(Deviation)
- 매직넘버 `#define`, 모든 변수 초기화, 명시적 캐스트, 단일 return(Rule 15.5),
  `#ifndef` 가드 불필요(단일 .c), 경계검사, 재귀 없음, 반환값 확인/`(void)` 캐스트.
- **이탈(사유 명시)**: 본 예제는 Linux 유저공간 전용(라이브러리 자체가 Linux/I2C 의존).
  - `Rule 21.6` (stdio): `printf`/`snprintf` — 진단 출력·프레임 조립에 필요.
  - POSIX API(`nanosleep`, `clock_gettime`, `sigaction`): 1초 주기·안전 종료에 필요.
  각 사용부에 `/* MISRA-C:2012 Rule 21.6 deviation: Linux 유저공간 진단 I/O */` 주석.

### 4.3 `eps_beacon.c` (전체)

```c
/******************************************************************************
 * eps_beacon.c — EPSBAT 전체 텔레메트리 1초 주기 AX.25 송출 예제
 *
 * 공개 API 레퍼런스만 사용: epsbat.h(EPS_*) + txu.h(TXU_*).
 *   To  (지상국) : ITSDGS-0    From (위성) : ITSSAT-0
 *   콜사인은 TXU_TrsSendFrameOverride 로 프레임마다 지정(영구 변경 없음).
 *
 * (!) 경고: TXU_TrsSendFrameOverride 는 실제 RF 를 송출한다(HIGH RISK).
 *     운용 규제·주파수 허가·안테나 정합을 확인한 환경에서만 TX_ENABLE=1 로 실행하라.
 *
 * 빌드: Makefile 참조 (libepsbat + libtxu 동시 링크). Linux/ARM 타겟 전용.
 * MISRA-C:2012 준수. 이탈 항목은 사용부 주석 참조.
 ******************************************************************************/
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#include "epsbat.h"
#include "txu.h"

/* ── 설정 상수 ─────────────────────────────────────────────────────────── */
#define TX_ENABLE            (1)        /* 0 = dry-run(송출 없이 출력만) */
#define TLM_PERIOD_SEC       (1L)       /* 목표 송출 주기(초) */
#define NS_PER_SEC           (1000000000L)
#define FRAME_MAX            (200U)     /* 프레임당 최대 payload byte */
#define TLM_BUF_SIZE         (512U)     /* 전체 텔레메트리 문자열 버퍼 */
#define EPS_ERR_U16          (0xFFFFU)  /* EPS 조회 오류 sentinel */
#define EPS_VER_OK           (0x0001U)  /* 정상 펌웨어 버전 */
#define CS_LEN               (7U)       /* == TXU_CALLSIGN_LEN */
#define PDM_COUNT            (8U)       /* 지원 PDM 채널 수 */
#define VSW1_CODE            (0xE410U)  /* PDM 채널1 전압 코드 */
#define VSW_CH_STEP          (16U)      /* 채널당 코드 증가폭 */
#define VSW_I_OFFSET         (4U)       /* 전류 코드 = 전압 코드 + 4 */

/* 온도 변환: 'C = TEMP_SCALE*count - TEMP_OFFSET */
#define TEMP_SCALE           (0.372434)
#define TEMP_OFFSET          (273.15)

/* ── 콜사인 (6 ASCII + 1 SSID) ─────────────────────────────────────────── */
static const uint8_t GS_CALLSIGN[CS_LEN]  = { 'I','T','S','D','G','S', 0U }; /* To  : ITSDGS-0 */
static const uint8_t SAT_CALLSIGN[CS_LEN] = { 'I','T','S','S','A','T', 0U }; /* From: ITSSAT-0 */

/* ── 버스 텔레메트리 정의(코드/라벨/스케일) ───────────────────────────── */
typedef struct { uint16_t code; const char *label; double scale; } bus_item_t;

static const bus_item_t BUS_ITEMS[] = {
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

/* PDM 채널 번호 / 전압·전류 스케일(레일별) */
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

/* ── 문자열 append 헬퍼(경계 안전, snprintf 기반) ──────────────────────── */
/* MISRA-C:2012 Rule 21.6 deviation: Linux 유저공간 진단/프레임 조립 I/O */
static size_t buf_append(char *buf, size_t size, size_t used, const char *fmt,
                         double val, uint8_t ok, const char *key)
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
            if (result > size) { result = size; }
        }
    }
    return result;
}

/* 정수(카운터/상태) append */
static size_t buf_append_u(char *buf, size_t size, size_t used, const char *key,
                           uint16_t val)
{
    size_t result = used;
    if (used < size)
    {
        int n = snprintf(&buf[used], size - used, "%s=%u ", key, (unsigned int)val);
        if (n > 0)
        {
            result = used + (size_t)n;
            if (result > size) { result = size; }
        }
    }
    return result;
}

/* ── 전체 텔레메트리 조립 → buf, 반환: 사용 길이 ──────────────────────── */
static size_t build_telemetry(char *buf, size_t size, uint32_t seq)
{
    size_t used = 0U;
    uint16_t i = 0U;

    /* 헤더 + 상태워드 */
    {
        int n = snprintf(buf, size, "ITSSAT TLM SEQ=%lu ST=0x%04X ",
                         (unsigned long)seq, (unsigned int)EPS_GetBoardStatus());
        if (n > 0) { used = (size_t)n; if (used > size) { used = size; } }
    }

    /* 전원 버스 8종 */
    for (i = 0U; i < (uint16_t)BUS_COUNT; i++)
    {
        uint8_t  ok  = 0U;
        uint16_t raw = EPS_GetTelemetry(BUS_ITEMS[i].code);
        double   phy = 0.0;
        if (raw != EPS_ERR_U16) { ok = 1U; phy = (double)raw * BUS_ITEMS[i].scale; }
        used = buf_append(buf, size, used, "%s=%.2f ", phy, ok, BUS_ITEMS[i].label);
    }

    /* 보드 온도 2종 */
    {
        const uint16_t tcode[2] = { 0xE308U, 0xE388U };
        const char    *tlabel[2] = { "T1", "T2" };
        uint16_t k = 0U;
        for (k = 0U; k < 2U; k++)
        {
            uint8_t  ok  = 0U;
            uint16_t raw = EPS_GetTelemetry(tcode[k]);
            double   phy = 0.0;
            if (raw != EPS_ERR_U16)
            {
                ok = 1U;
                phy = (TEMP_SCALE * (double)raw) - TEMP_OFFSET;
            }
            used = buf_append(buf, size, used, "%s=%.1f ", phy, ok, tlabel[k]);
        }
    }

    /* PDM 채널 텔레메트리(1~6,8,9) 각 V/I */
    for (i = 0U; i < (uint16_t)PDM_COUNT; i++)
    {
        char     klv[8] = { 0 };
        char     kli[8] = { 0 };
        uint16_t vcode  = (uint16_t)(VSW1_CODE
                          + ((uint16_t)VSW_CH_STEP * (uint16_t)(PDM_CH[i] - 1U)));
        uint16_t icode  = (uint16_t)(vcode + VSW_I_OFFSET);
        uint16_t vraw   = EPS_GetTelemetry(vcode);
        uint16_t iraw   = EPS_GetTelemetry(icode);
        uint8_t  vok    = (vraw != EPS_ERR_U16) ? 1U : 0U;
        uint8_t  iok    = (iraw != EPS_ERR_U16) ? 1U : 0U;
        double   vphy   = (vok != 0U) ? ((double)vraw * PDM_VSCALE[i]) : 0.0;
        double   iphy   = (iok != 0U) ? ((double)iraw * PDM_ISCALE[i]) : 0.0;

        (void)snprintf(klv, sizeof(klv), "P%uV", (unsigned int)PDM_CH[i]);
        (void)snprintf(kli, sizeof(kli), "P%uI", (unsigned int)PDM_CH[i]);
        used = buf_append(buf, size, used, "%s=%.2f ", vphy, vok, klv);
        used = buf_append(buf, size, used, "%s=%.3f ", iphy, iok, kli);
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
        if (rc != 0) { status = -1; }
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
        long sec  = (long)now.tv_sec  - (long)start->tv_sec;
        long nsec = now.tv_nsec - start->tv_nsec;
        long elapsed_ns = (sec * NS_PER_SEC) + nsec;
        long target_ns  = TLM_PERIOD_SEC * NS_PER_SEC;
        if (elapsed_ns < target_ns)
        {
            struct timespec req = { 0, 0 };
            long rem_ns = target_ns - elapsed_ns;
            req.tv_sec  = rem_ns / NS_PER_SEC;
            req.tv_nsec = rem_ns % NS_PER_SEC;
            (void)nanosleep(&req, NULL);
        }
    }
}

/* ── main ──────────────────────────────────────────────────────────────── */
int main(void)
{
    int             exit_code = 0;
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
            char   frame[TLM_BUF_SIZE];
            size_t len = 0U;
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
```

### 4.4 `eps_beacon.py` (ctypes, 전체)

```python
#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""EPSBAT 전체 텔레메트리를 1초 주기로 AX.25 프레임(To=ITSDGS-0, From=ITSSAT-0)에
실어 TXU 로 송출하는 예제. 공개 API(libepsbat.so + libtxu.so)만 사용.

(!) TXU_TrsSendFrameOverride 는 실제 RF 를 송출한다(HIGH RISK).
    운용 규제/주파수 허가 확인된 환경에서만 TX_ENABLE=True 로 실행하라.
"""
import ctypes as C
import signal
import sys
import time

TX_ENABLE   = True          # False = dry-run(송출 없이 출력)
PERIOD_SEC  = 1.0
FRAME_MAX   = 200
EPS_ERR_U16 = 0xFFFF

# ── 라이브러리 로드(실행 파일 옆에 .so 배치) ──────────────────────────────
eps = C.CDLL("./libepsbat.so")
txu = C.CDLL("./libtxu.so")

# EPS 시그니처
eps.EPS_GetVersion.restype     = C.c_uint16
eps.EPS_GetBoardStatus.restype = C.c_uint16
eps.EPS_GetTelemetry.restype   = C.c_uint16
eps.EPS_GetTelemetry.argtypes  = [C.c_uint16]
for fn in ("EPS_GetNumberOfBrownOutReset", "EPS_GetNumberOfAutomaticReset",
           "EPS_GetNumberOfManuelReset", "EPS_GetNumberOfCommWatchdogReset"):
    getattr(eps, fn).restype = C.c_uint16

# TXU 시그니처: int TXU_TrsSendFrameOverride(u8 to[7],u8 from[7],u8* frame,u8 len,u8* rem)
CS7 = C.c_ubyte * 7
txu.TXU_TrsSendFrameOverride.restype  = C.c_int
txu.TXU_TrsSendFrameOverride.argtypes = [CS7, CS7, C.POINTER(C.c_ubyte),
                                         C.c_uint8, C.POINTER(C.c_ubyte)]
txu.TXU_Close.restype = None

# ── 콜사인(6 ASCII + 1 SSID) ──────────────────────────────────────────────
TO_CS   = CS7(*b"ITSDGS", 0)   # 지상국 ITSDGS-0
FROM_CS = CS7(*b"ITSSAT", 0)   # 위성   ITSSAT-0

# ── 텔레메트리 정의 ───────────────────────────────────────────────────────
BUS = [  # (code, label, scale)
    (0xE220, "VB",  0.008978), (0xE224, "IB",  0.00681989),
    (0xE230, "V12", 0.01349),  (0xE234, "I12", 0.00206663),
    (0xE210, "V5",  0.005865), (0xE214, "I5",  0.00681989),
    (0xE200, "V3",  0.004311), (0xE204, "I3",  0.00681989),
]
TEMP  = [(0xE308, "T1"), (0xE388, "T2")]  # 'C = 0.372434*count - 273.15
VSW1  = 0xE410
PDM   = [  # (ch, vscale, iscale)
    (1, 0.01349, 0.0013283), (2, 0.01349, 0.0013283),
    (3, 0.008993, 0.0062395), (4, 0.008993, 0.0062395),
    (5, 0.005865, 0.0013288), (6, 0.005865, 0.0013288),
    (8, 0.004311, 0.0013284), (9, 0.004311, 0.0013284),
]

_run = True
def _stop(sig, frm):
    global _run
    _run = False
signal.signal(signal.SIGINT, _stop)


def read_scaled(code, scale, fmt):
    raw = eps.EPS_GetTelemetry(code)
    return "ERR" if raw == EPS_ERR_U16 else fmt % (raw * scale)


def read_temp(code):
    raw = eps.EPS_GetTelemetry(code)
    return "ERR" if raw == EPS_ERR_U16 else "%.1f" % (0.372434 * raw - 273.15)


def build_telemetry(seq):
    p = ["ITSSAT TLM SEQ=%d ST=0x%04X" % (seq, eps.EPS_GetBoardStatus())]
    for code, label, scale in BUS:
        p.append("%s=%s" % (label, read_scaled(code, scale, "%.2f")))
    for code, label in TEMP:
        p.append("%s=%s" % (label, read_temp(code)))
    for ch, vs, is_ in PDM:
        vcode = VSW1 + 16 * (ch - 1)
        p.append("P%dV=%s" % (ch, read_scaled(vcode, vs, "%.2f")))
        p.append("P%dI=%s" % (ch, read_scaled(vcode + 4, is_, "%.3f")))
    p.append("BOR=%d" % eps.EPS_GetNumberOfBrownOutReset())
    p.append("AR=%d"  % eps.EPS_GetNumberOfAutomaticReset())
    p.append("MR=%d"  % eps.EPS_GetNumberOfManuelReset())
    p.append("CWR=%d" % eps.EPS_GetNumberOfCommWatchdogReset())
    return " ".join(p).encode("ascii")


def send_frame(payload):
    for off in range(0, len(payload), FRAME_MAX):
        chunk = payload[off:off + FRAME_MAX]
        buf = (C.c_ubyte * len(chunk)).from_buffer_copy(chunk)
        if TX_ENABLE:
            rem = C.c_ubyte(0)
            rc = txu.TXU_TrsSendFrameOverride(TO_CS, FROM_CS, buf,
                                              len(chunk), C.byref(rem))
            if rc != 0:
                return -1
        else:
            print("[DRY-RUN TX %d B] %s" % (len(chunk), chunk.decode("ascii")))
    return 0


def main():
    print("=== EPSBAT TLM beacon  To=ITSDGS-0  From=ITSSAT-0 ===")
    print("(!) RF 송출" if TX_ENABLE else "(dry-run)", "— Ctrl-C 로 중단")
    if eps.EPS_GetVersion() != 0x0001:
        print("EPS board not found (version mismatch)."); return 1
    seq = 0
    while _run:
        t0 = time.monotonic()
        if send_frame(build_telemetry(seq)) != 0:
            print("TX error (seq=%d)" % seq)
        seq += 1
        dt = PERIOD_SEC - (time.monotonic() - t0)
        if dt > 0:
            time.sleep(dt)
    print("\nstopped by user.")
    txu.TXU_Close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
```

### 4.5 `Makefile`

```make
# EPSBAT TLM beacon 예제 — libepsbat + libtxu 동시 링크 (Linux/ARM 타겟)
CC      ?= gcc
CFLAGS  ?= -std=c99 -Wall -Wextra -O2
# 헤더/라이브러리 경로(배포 폴더 기준, 필요시 수정)
EPS_DIR ?= ../../Release_ITSDL_EPSBAT_20260712/library
TXU_DIR ?= ../../ITSDL_TXU_TEST_CODE
INCLUDES = -I$(EPS_DIR) -I$(TXU_DIR)

# 정적 링크 권장(라이브러리 .a 를 실행 파일에 포함)
STATIC_LIBS = $(EPS_DIR)/libepsbat.a $(TXU_DIR)/libtxu.a

eps_beacon: eps_beacon.c
	$(CC) $(CFLAGS) $(INCLUDES) eps_beacon.c $(STATIC_LIBS) -o eps_beacon

# 공유 링크 대안:
#   $(CC) $(CFLAGS) $(INCLUDES) eps_beacon.c -L$(EPS_DIR) -L$(TXU_DIR) \
#         -lepsbat -ltxu -Wl,-rpath,'$$ORIGIN' -o eps_beacon

clean:
	rm -f eps_beacon

.PHONY: clean
```

### 4.6 `README.md` (요지)
- 개요, To/From 콜사인, 전송 항목, 빌드(`make`), 실행(`sudo ./eps_beacon`), dry-run
  전환(`TX_ENABLE`), 안전 경고(RF 송출), 주기 한계(수백 ms/호출) 명시.

---

## 5. 예상 영향 범위 및 리스크

- 신규 파일만 추가, 기존 코드 무변경 → 회귀 리스크 없음.
- **RF 송출(HIGH)**: `TX_ENABLE=1` 실행 시 실제 송신. 미허가 송출 방지 위해 기본
  코드는 그대로 두되, 검증 전에는 `TX_ENABLE=0`(dry-run) 권장을 README/주석에 명시.
- **콜사인 영구 변경 없음**: `SendFrameOverride` 만 사용 → 보드 저장 콜사인 불변.
- **PDM 채널 스케일**: PDF 디코딩 기반(§3.3 경고). 버스/온도 스케일은 헤더와 일치 확인됨.
- **주기**: 전체 조회가 1초를 넘으면 실측 주기 > 1초(정직하게 문서화).
- 라이브러리 `.a/.so` 는 타겟에서 `make dist` 후 존재해야 빌드/실행 가능
  (개발 PC/Windows 빌드 불가 — README 명시).

## 6. 완료 기준 (검증 방법)

1. **빌드**: 타겟에서 `make` 성공(경고 0 목표), `eps_beacon` 생성.
2. **Dry-run**: `TX_ENABLE=0`(C)/`False`(Py) 로 실행 시 1초 주기로 전체 항목
   ASCII 프레임이 출력되고, SEQ 증가, `Ctrl-C` 정상 종료.
3. **필드 확인**: 프레임에 VB/IB/V12/I12/V5/I5/V3/I3, T1/T2, P{1..6,8,9}V/I,
   BOR/AR/MR/CWR, ST 가 모두 포함.
4. **송출(권한 환경)**: `TX_ENABLE=1` + 스펙트럼 분석기/수신기로 AX.25 프레임의
   To=ITSDGS-0 / From=ITSSAT-0 확인.
5. Python 예제도 동일 출력/동작 확인.

---

## 7. 승인 요청

위 계획대로 `Examples/eps_tlm_beacon/` 4개 파일을 생성해도 될지 확인 부탁드립니다.
특히 아래 두 가지 기본값에 대한 확인이 필요합니다:
- 기본 `TX_ENABLE` 값을 **0(dry-run, 안전)** 으로 둘지, **1(실제 송출)** 로 둘지.
- PDM 채널 스케일을 디코딩값 그대로 사용할지, 아니면 PDM 채널은 **원시 ADC count**
  로만 실어 스케일 오차 리스크를 제거할지.
```
