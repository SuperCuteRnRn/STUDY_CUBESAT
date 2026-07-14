/******************************************************************************
 * txu_test.c — ITSDL_TRXVU 통신보드 통합 메뉴 테스트 (flat API)
 *
 * open/close 불필요 — TXU_* 함수만 호출한다. 위험 명령은 확인 프롬프트 후 실행.
 *  - slave: RCV 0x60, TRS 0x61
 *  - 메뉴 노출: read-only 조회 위주 + Reset(확인). RF 송출·주파수 변경·콜사인
 *    영구 변경은 API(TXU_*)로 코드에서 직접 사용 가능하되 메뉴 미노출(안전 정책).
 *
 * 빌드: make      실행: sudo ./txu_test
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "txu.h"

static int read_int(const char *prompt)
{
    char buf[64];
    printf("%s", prompt);
    fflush(stdout);
    if (fgets(buf, sizeof(buf), stdin) == NULL) { return -1; }
    return atoi(buf);
}

static int confirm(const char *warn)
{
    char buf[16];
    printf("  (!) %s — 실행하려면 YES 입력: ", warn);
    fflush(stdout);
    if (fgets(buf, sizeof(buf), stdin) == NULL) { return 0; }
    return (strncmp(buf, "YES", 3) == 0) ? 1 : 0;
}

static void hexdump(const char *label, const uint8_t *buf, int n)
{
    int i;
    printf("  %s (%d B):", label, n);
    for (i = 0; i < n; i++) { printf(" %02X", buf[i]); }
    printf("\n");
}

/* 1) 보드 정보 (uptime/firmware/last reset) */
static void test_board_info(void)
{
    char fw[80];
    uint32_t up;
    uint8_t cause;

    printf("\n[보드 정보]\n");
    up = TXU_GetUptime(TXU_TRS);
    if (up != TXU_ERR_U32) { printf("  TRS Uptime   : %u s\n", up); }
    else { printf("  TRS Uptime   : ERROR\n"); }

    if (TXU_GetFirmwareInfo(TXU_TRS, fw) == 0) { printf("  TRS Firmware : %s\n", fw); }
    else { printf("  TRS Firmware : ERROR\n"); }

    if (TXU_GetLastResetCause(TXU_TRS, &cause) == 0)
        printf("  TRS LastReset: 0x%02X\n", cause);
    else
        printf("  TRS LastReset: ERROR\n");
}

/* 2) RCV/TRS state + telemetry 원시 덤프 */
static void test_state_telemetry(void)
{
    uint8_t st;
    uint8_t tlm[16];
    int n;

    printf("\n[상태/텔레메트리]\n");
    if (TXU_TrsGetState(&st) == 0) { printf("  TRS State    : 0x%02X\n", st); }
    else { printf("  TRS State    : ERROR\n"); }

    n = TXU_TrsGetTelemetry(tlm);
    if (n > 0) { hexdump("TRS TLM", tlm, n); } else { printf("  TRS TLM: ERROR\n"); }

    n = TXU_RcvGetTelemetry(tlm);
    if (n > 0) { hexdump("RCV TLM", tlm, n); } else { printf("  RCV TLM: ERROR\n"); }
}

/* 3) RX 프레임 큐 조회 */
static void test_rx_frames(void)
{
    uint16_t cnt = 0U;
    uint16_t len = 0U;

    printf("\n[RX 프레임 큐]\n");
    if (TXU_RcvGetFrameCount(&cnt) == 0)  { printf("  프레임 수 : %u\n", cnt); }
    else { printf("  프레임 수 : ERROR\n"); }
    if (TXU_RcvGetFrameLength(&len) == 0) { printf("  최신 길이 : %u\n", len); }
    else { printf("  최신 길이 : ERROR\n"); }
}

/* 4) PLL 오류 조회 */
static void test_pll(void)
{
    uint16_t lock = 0U;
    uint16_t freq = 0U;

    printf("\n[PLL 오류]\n");
    if (TXU_GetPllError(TXU_TRS, &lock, &freq) == 0)
        printf("  TRS lock_err=%u freq_err=%u\n", lock, freq);
    else
        printf("  TRS PLL : ERROR\n");
}

/* 5) 콜사인 조회 */
static void test_callsign(void)
{
    uint8_t to[TXU_CALLSIGN_LEN];
    uint8_t fr[TXU_CALLSIGN_LEN];

    printf("\n[AX.25 콜사인]\n");
    if (TXU_TrsGetToCallsign(to) == 0)   { hexdump("TO  ", to, (int)TXU_CALLSIGN_LEN); }
    else { printf("  TO   : ERROR\n"); }
    if (TXU_TrsGetFromCallsign(fr) == 0) { hexdump("FROM", fr, (int)TXU_CALLSIGN_LEN); }
    else { printf("  FROM : ERROR\n"); }
}

/* 6) Transponder RSSI 조회 */
static void test_transponder(void)
{
    uint16_t rssi = 0U;

    printf("\n[Transponder]\n");
    if (TXU_GetTransponderRssi(&rssi) == 0) { printf("  RSSI raw : %u\n", rssi); }
    else { printf("  RSSI : ERROR\n"); }
}

/* 7) TX Idle ON/OFF */
static void test_idle(void)
{
    int on = read_int("Idle 1=ON, 0=OFF: ");
    if (TXU_TrsSetIdle((uint8_t)((on != 0) ? 1 : 0)) == 0) { printf("  전송 완료.\n"); }
    else { printf("  ERROR\n"); }
}

/* 8) TX Bitrate 설정 */
static void test_bitrate(void)
{
    int b = read_int("Bitrate mask (1=1200,2=2400,4=4800,8=9600): ");
    if (TXU_TrsSetBitrate((uint8_t)b) == 0) { printf("  전송 완료.\n"); }
    else { printf("  ERROR\n"); }
}

/* 80) Reset (위험) */
static void test_reset(void)
{
    int sel = read_int("리셋 대상 (0=RCV, 1=TRS): ");
    TXU_Slave sl = (sel == 1) ? TXU_TRS : TXU_RCV;

    if (confirm("Watchdog Reset — 보드 리셋") != 0)
    {
        if (TXU_WatchdogReset(sl) == 0) { printf("  리셋 명령 전송.\n"); }
        else { printf("  ERROR\n"); }
    }
    else { printf("  취소됨.\n"); }
}

/* 90) DEV-001 자동 비교 시험 */
static void test_dev001(void)
{
    int r = TXU_RunDev001Test();
    printf("\n  DEV-001 결과 코드: %d (0=결정, -1=baseline 실패, -2=ambiguous)\n", r);
}

int main(void)
{
    printf("=====================================================\n");
    printf(" ITSDL TRXVU 통신보드 통합 테스트 (RCV 0x60 / TRS 0x61)\n");
    printf(" flat API — open/close 불필요\n");
    printf("=====================================================\n");

    for (;;)
    {
        int sel;

        printf("\n"
               " 1) 보드 정보 (uptime/firmware/last reset)\n"
               " 2) 상태/텔레메트리 덤프\n"
               " 3) RX 프레임 큐 조회\n"
               " 4) PLL 오류 조회\n"
               " 5) AX.25 콜사인 조회\n"
               " 6) Transponder RSSI 조회\n"
               " 7) TX Idle ON/OFF\n"
               " 8) TX Bitrate 설정\n"
               "80) Reset (위험 — 확인 프롬프트)\n"
               "90) DEV-001 transponder freq 자동 비교 시험\n"
               " 0) 종료\n");
        sel = read_int("선택: ");
        switch (sel)
        {
        case 1:  test_board_info();      break;
        case 2:  test_state_telemetry(); break;
        case 3:  test_rx_frames();       break;
        case 4:  test_pll();             break;
        case 5:  test_callsign();        break;
        case 6:  test_transponder();     break;
        case 7:  test_idle();            break;
        case 8:  test_bitrate();         break;
        case 80: test_reset();           break;
        case 90: test_dev001();          break;
        case 0:  TXU_Close(); return 0;
        default: printf("  잘못된 선택입니다.\n"); break;
        }
    }
}
