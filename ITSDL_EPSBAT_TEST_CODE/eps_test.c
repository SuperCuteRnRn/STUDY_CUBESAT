/******************************************************************************
 * eps_test.c — ITSDL BAT 보드(STM32 펌웨어) 통합 테스트 프로그램
 *
 * 펌웨어가 지원하는 모든 I2C 명령을 하나의 메뉴에서 테스트한다.
 *  - ITSDL_EPS&BAT 명령 체계, 슬레이브 주소 0x2B
 *  - PDM 지원 채널: 1/2=12V, 3/4=BAT, 5/6=5V, 8/9=3.3V
 *
 * 빌드: make          실행: ./eps_test
 ******************************************************************************/
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "stdint.h"
#include "function.h"
#include "i2c_lib.h"

/* 지원 채널 목록 */
static const int VALID_SW[] = {1, 2, 3, 4, 5, 6, 8, 9};
#define VALID_SW_NUM 8

static const char *SW_NAME[11] = {"",
    "12V PDM1", "12V PDM2", "BAT PDM1", "BAT PDM2",
    "5V PDM1",  "5V PDM2",  "-",        "3V3 PDM1",
    "3V3 PDM2", "-"};

/* 채널별 전압/전류 스케일 */
static double vsw_scale(int sw)
{
    if (sw <= 2) return 0.01349;   /* 12V */
    if (sw <= 4) return 0.008993;  /* BAT */
    if (sw <= 6) return 0.005865;  /* 5V  */
    return 0.004311;               /* 3V3 */
}
static double isw_scale(int sw)
{
    return (sw == 3 || sw == 4) ? 0.006239 : 0.001328; /* BAT만 고전류 스케일 */
}

/* 0xFFFF(에러/busy) 여부 표시용 */
static void print_val(const char *label, uint16_t raw, double scale,
                      const char *unit)
{
    if (raw == 0xffff)
        printf("  %-14s : ERROR(0xFFFF)\n", label);
    else
        printf("  %-14s : %8.4f %-4s (raw %4u)\n", label, raw * scale, unit,
               raw);
}

static int read_int(const char *prompt)
{
    char buf[64];
    printf("%s", prompt);
    fflush(stdout);
    if (fgets(buf, sizeof(buf), stdin) == NULL)
        return -1;
    return atoi(buf);
}

/* ── 1. 보드 정보 ─────────────────────────────────────────── */
static void test_board_info(void)
{
    printf("\n[보드 정보]\n");
    printf("  Board Status : 0x%04X (Mother<<8|Daughter, 정상=0x0000)\n",
           EPS_GetBoardStatus());
    printf("  Version      : 0x%04X (기대값 0x0001)\n", EPS_GetVersion());
    printf("  Checksum     : 0x%04X (기대값 0xA5A5)\n", EPS_GetChecksum());
    printf("  Revision     : 0x%04X (기대값 0x0001)\n", EPS_GetRevision());
    printf("  Last Error   : 0x%04X (0x01=명령, 0x02=데이터, 0x03=채널)\n",
           EPS_GetLastError());
}

/* ── 2. 전체 텔레메트리 1회 ──────────────────────────────── */
static void test_telemetry_once(void)
{
    printf("\n[PCM 텔레메트리]\n");
    print_val("VPCM12V",  EPS_GetTelemetry(EPS_TLE_VPCM12V),  0.01349,        "V");
    print_val("IPCM12V",  EPS_GetTelemetry(EPS_TLE_IPCM12V),  0.002066632361, "A");
    print_val("VPCMBATV", EPS_GetTelemetry(EPS_TLE_VPCMBATV), 0.008978,       "V");
    print_val("IPCMBATV", EPS_GetTelemetry(EPS_TLE_IPCMBATV), 0.00681988679,  "A");
    print_val("VPCM5V",   EPS_GetTelemetry(EPS_TLE_VPCM5V),   0.005865,       "V");
    print_val("IPCM5V",   EPS_GetTelemetry(EPS_TLE_IPCM5V),   0.00681988679,  "A");
    print_val("VPCM3V3",  EPS_GetTelemetry(EPS_TLE_VPCM3V3),  0.004311,       "V");
    print_val("IPCM3V3",  EPS_GetTelemetry(EPS_TLE_IPCM3V3),  0.00681988679,  "A");

    printf("\n[PDM 스위치 텔레메트리]\n");
    for (int i = 0; i < VALID_SW_NUM; i++) {
        int sw = VALID_SW[i];
        char label[32];
        uint16_t v = EPS_GetTelemetry(EPS_TLE_VSW1 + 16 * (sw - 1));
        uint16_t c = EPS_GetTelemetry(EPS_TLE_VSW1 + 16 * (sw - 1) + 4);
        snprintf(label, sizeof(label), "SW%d %s", sw, SW_NAME[sw]);
        if (v == 0xffff || c == 0xffff)
            printf("  %-14s : ERROR(0xFFFF)\n", label);
        else
            printf("  %-14s : %7.3f V, %7.3f A\n", label, v * vsw_scale(sw),
                   c * isw_scale(sw));
    }

    printf("\n[온도]\n");
    uint16_t t;
    t = EPS_GetTelemetry(EPS_TLE_TBRD);
    printf("  TBRD (센서0)   : %7.2f 'C\n", t * 0.372434 - 273.15);
    t = EPS_GetTelemetry(EPS_TLE_TLM_TBRD_DB);
    printf("  TBRD_DB(센서1) : %7.2f 'C\n", t * 0.372434 - 273.15);
}

/* ── 3. 반복 모니터링 ────────────────────────────────────── */
static void test_repeat_monitor(void)
{
    int n = read_int("반복 횟수 입력: ");
    if (n <= 0)
        return;
    for (int i = 0; i < n; i++) {
        double v12 = EPS_GetTelemetry(EPS_TLE_VPCM12V) * 0.01349;
        double i12 = EPS_GetTelemetry(EPS_TLE_IPCM12V) * 0.002066632361;
        double vb  = EPS_GetTelemetry(EPS_TLE_VPCMBATV) * 0.008978;
        double ib  = EPS_GetTelemetry(EPS_TLE_IPCMBATV) * 0.00681988679;
        double v5  = EPS_GetTelemetry(EPS_TLE_VPCM5V) * 0.005865;
        double i5  = EPS_GetTelemetry(EPS_TLE_IPCM5V) * 0.00681988679;
        double v3  = EPS_GetTelemetry(EPS_TLE_VPCM3V3) * 0.004311;
        double i3  = EPS_GetTelemetry(EPS_TLE_IPCM3V3) * 0.00681988679;
        printf("[%3d] 12V %.2fV/%.3fA | BAT %.2fV/%.3fA | "
               "5V %.2fV/%.3fA | 3V3 %.2fV/%.3fA\n",
               i + 1, v12, i12, vb, ib, v5, i5, v3, i3);
    }
}

/* ── 4~7. PDM 제어 ──────────────────────────────────────── */
static void pdm_bitmap_print(const char *name, uint16_t bm)
{
    printf("  %-8s = 0x%04X : ", name, bm);
    if (bm == 0xffff) {
        printf("ERROR\n");
        return;
    }
    for (int i = 0; i < VALID_SW_NUM; i++) {
        int sw = VALID_SW[i];
        printf("SW%d=%s ", sw, (bm >> (sw - 1)) & 1 ? "ON" : "off");
    }
    printf("\n");
}

static void test_pdm_switch(int on)
{
    int ch = read_int(on ? "켤 채널 (1-6, 8, 9): " : "끌 채널 (1-6, 8, 9): ");
    if (!EPS_IsValidPDMChannel(ch)) {
        printf("  잘못된 채널입니다 (지원: 1-6, 8, 9).\n");
        return;
    }
    if (on)
        EPS_SwitchNPDMOn(ch);
    else
        EPS_SwitchNPDMOff(ch);
    usleep(300000);
    printf("  SW%d(%s) -> %s 명령 전송. 상태: %u\n", ch, SW_NAME[ch],
           on ? "ON" : "OFF", EPS_GetActualStateOfNPDM(ch));
}

static void test_pdm_status(void)
{
    printf("\n[PDM 상태]\n");
    pdm_bitmap_print("Actual",   EPS_GetActualStateOfAllPDM());
    pdm_bitmap_print("Expected", EPS_GetExpectedStateOfAllPDM());
    pdm_bitmap_print("Initial",  EPS_GetInitialStateOfAllPDM());
}

static void test_pdm_initial(void)
{
    int ch = read_int("초기상태 설정할 채널 (1-6, 8, 9): ");
    if (!EPS_IsValidPDMChannel(ch)) {
        printf("  잘못된 채널입니다.\n");
        return;
    }
    int on = read_int("1=초기 ON, 0=초기 OFF: ");
    if (on)
        EPS_SetInitialStateOfNPDMOn(ch);
    else
        EPS_SetInitialStateOfNPDMOff(ch);
    usleep(300000); /* 펌웨어 busy 200ms */
    pdm_bitmap_print("Initial", EPS_GetInitialStateOfAllPDM());
}

/* ── 8. 리셋 카운터 (RCC_CSR 리셋 원인별 실측) ──────────── */
static void test_reset_counters(void)
{
    printf("\n[리셋 카운터] (부팅 시 RCC_CSR 리셋 원인 기록)\n");
    printf("  Brownout(IWDG) : %u\n", EPS_GetNumberOfBrownOutReset());
    printf("  Auto SW        : %u\n", EPS_GetNumberOfAutomaticReset());
    printf("  Manual(PIN)    : %u\n", EPS_GetNumberOfManuelReset());
    printf("  Power-on(POR)  : %u\n", EPS_GetNumberOfCommWatchdogReset());
}

/* ── 메뉴 ───────────────────────────────────────────────── */
int main(void)
{
    printf("=====================================================\n");
    printf(" ITSDL EPS BAT 보드 통합 테스트 (주소 0x2B)\n");
    printf(" PDM 채널: 1/2=12V, 3/4=BAT, 5/6=5V, 8/9=3V3\n");
    printf("=====================================================\n");

    for (;;) {
        printf("\n"
               " 1) 보드 정보 (Status/Version/Checksum/Revision)\n"
               " 2) 전체 텔레메트리 1회 출력\n"
               " 3) PCM 반복 모니터링\n"
               " 4) PDM 개별 ON\n"
               " 5) PDM 개별 OFF\n"
               " 6) PDM 전체 ON\n"
               " 7) PDM 전체 OFF\n"
               " 8) PDM 상태 조회 (Actual/Expected/Initial)\n"
               " 9) PDM 초기상태 설정\n"
               "10) 리셋 카운터 조회\n"
               " 0) 종료\n");
        int sel = read_int("선택: ");
        switch (sel) {
        case 1:  test_board_info();      break;
        case 2:  test_telemetry_once();  break;
        case 3:  test_repeat_monitor();  break;
        case 4:  test_pdm_switch(1);     break;
        case 5:  test_pdm_switch(0);     break;
        case 6:
            EPS_SwitchAllPDMOn();
            usleep(300000);
            test_pdm_status();
            break;
        case 7:
            EPS_SwitchAllPDMOff();
            usleep(300000);
            test_pdm_status();
            break;
        case 8:  test_pdm_status();      break;
        case 9:  test_pdm_initial();     break;
        case 10: test_reset_counters();  break;
        case 0:  return 0;
        default: printf("  잘못된 선택입니다.\n"); break;
        }
    }
}
