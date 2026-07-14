/* ============================================================================
 * trxvu_dev001.c — DEV-001 (MISRA Rule 1.3 deviation) wire-level 자동 비교 시험
 * (PLAN_TRX_HW_TEST_STANDALONE_20260521.md, 2026-05-21)
 *
 * ICD-0002 §5.1.1.4/5 의 명령 코드 불일치 (heading 0x80/0x81 vs table 0x32/0x33)
 * 를 4-조합 매트릭스로 결정한다.
 *
 * DEV-001 RESOLVED (2026-05-21): default = 0x32/0x33 (table). 0x80/0x81 = _ALT.
 *
 *   Combo  Set CC   Get CC   기대 시나리오
 *   -----  ------   ------   -------------
 *     A     0x32     0x33    ICD-0002 table 표기 (현 default 채택)
 *     B     0x32     0x81    cross check (table set + heading get)
 *     C     0x80     0x33    cross check (heading set + table get)
 *     D     0x80     0x81    ICD-0002 heading 표기 (legacy)
 *
 * 판정:
 *   - A match, D mismatch → default 0x32/0x33 정답 (유지)
 *   - All match → 동의어. default 0x32/0x33 유지
 *   - D only match → heading 표기가 정답 (ALT 로 swap 필요)
 *   - 대각선 매치 (A+D match, B+C mismatch) → 별도 명령. 정밀 wire 분석
 *   - All mismatch → 펌웨어/wire 분석 필요
 * ========================================================================== */
#include "trxvu_dev001.h"
#include "trxvu_commands.h"

#include <stdio.h>
#include <string.h>

/* 시험 대상 주파수 — VHF 아마추어 대역, 4 조합 별 변별 가능한 값 */
#define DEV001_FREQ_A   144500U   /* combo A test value */
#define DEV001_FREQ_B   145000U
#define DEV001_FREQ_C   145500U
#define DEV001_FREQ_D   146000U

/* 보드 settling delay */
#define DEV001_SETTLE_MS   500U
#define DEV001_INTRA_MS    200U

typedef struct
{
    uint8_t  set_cc;
    uint8_t  get_cc;
    uint32_t freq_sent;
    uint32_t freq_read;
    uint8_t  status;
    int      set_rc;
    int      get_rc;
    int      match;            /* 1 if read == sent and status == 0 */
} dev001_combo_t;

static void dev001_print_header(void)
{
    printf("\n============================================================\n");
    printf("DEV-001 wire-level test (ICD-0002 §5.1.1.4/5)\n");
    printf("Default CC: 0x32/0x33 (table) | Legacy CC: 0x80/0x81 (heading)\n");
    printf("============================================================\n");
}

static void dev001_print_result(const dev001_combo_t *c, int idx)
{
    const char *label = "?";
    if      (idx == 0) { label = "A (default/default 0x32-0x33)"; }
    else if (idx == 1) { label = "B (default set, legacy get)"; }
    else if (idx == 2) { label = "C (legacy set, default get)"; }
    else if (idx == 3) { label = "D (legacy/legacy 0x80-0x81)"; }

    printf(" %-30s | set 0x%02X | get 0x%02X | sent %6u | read %6u | st 0x%02X "
           "| set rc %3d | get rc %3d | %s\n",
           label,
           (unsigned)c->set_cc, (unsigned)c->get_cc,
           (unsigned)c->freq_sent, (unsigned)c->freq_read,
           (unsigned)c->status,
           c->set_rc, c->get_rc,
           c->match ? "MATCH" :
               ((c->status == 0xFFU) ? "miss(intl-err)" :
                (c->status == 0xFEU) ? "miss(read-err)" :
                (c->status == 0xFDU) ? "miss(busy)" :
                (c->status == 0xFCU) ? "miss(not-ready)" :
                (c->freq_read == 437265U) ? "miss(=TX_freq)" :
                "miss"));
}

int trxvu_dev001_run(trxvu_ctx_t *ctx)
{
    dev001_combo_t combos[4] = {
        { TRXVU_CC_SET_TRP_FREQ,     TRXVU_CC_GET_TRP_FREQ,     DEV001_FREQ_A, 0U, 0U, 0, 0, 0 },
        { TRXVU_CC_SET_TRP_FREQ,     TRXVU_CC_GET_TRP_FREQ_ALT, DEV001_FREQ_B, 0U, 0U, 0, 0, 0 },
        { TRXVU_CC_SET_TRP_FREQ_ALT, TRXVU_CC_GET_TRP_FREQ,     DEV001_FREQ_C, 0U, 0U, 0, 0, 0 },
        { TRXVU_CC_SET_TRP_FREQ_ALT, TRXVU_CC_GET_TRP_FREQ_ALT, DEV001_FREQ_D, 0U, 0U, 0, 0, 0 }
    };
    int     result = 0;
    int     i;
    uint8_t state_before = 0U;
    uint8_t state_after  = 0U;

    dev001_print_header();

    /* (1) Baseline: TRS state read (0x41) — 보드 응답 normal 확인 */
    if (trxvu_cmd_trs_get_state(ctx, &state_before) != 0)
    {
        fprintf(stderr, "DEV-001: baseline TRS state read failed — abort.\n");
        result = -1;
    }
    else
    {
        printf("Baseline TRS state: 0x%02X\n", (unsigned)state_before);

        /* (1b) PREREQUISITE: enable transponder mode (0x38 0x02)
         * ICD-0002 §2: "Transponder mode is only available if the option is
         * selected in the Option Sheet". If Option Sheet 미선택, this command
         * has no effect — 4-combo 결과로 판정 가능. */
        printf("Pre-enable transponder mode (0x38 0x02)...\n");
        if (trxvu_cmd_set_tx_mode(ctx, TRXVU_TX_MODE_TRANSPONDER) != 0)
        {
            fprintf(stderr, "WARN: 0x38 0x02 send failed (continuing test anyway).\n");
        }
        trxvu_sleep_ms(DEV001_SETTLE_MS);
        printf("(transponder enable settled)\n\n");

        /* (2) 4 조합 실행 */
        for (i = 0; i < 4; i++)
        {
            trxvu_sleep_ms(DEV001_SETTLE_MS);

            combos[i].set_rc = trxvu_cmd_set_trp_freq_via(
                ctx, combos[i].set_cc, combos[i].freq_sent);

            trxvu_sleep_ms(DEV001_INTRA_MS);

            combos[i].get_rc = trxvu_cmd_get_trp_freq_via(
                ctx, combos[i].get_cc,
                &combos[i].freq_read, &combos[i].status);

            combos[i].match = ((combos[i].set_rc == 0)
                              && (combos[i].get_rc == 0)
                              && (combos[i].freq_read == combos[i].freq_sent)
                              && (combos[i].status == 0U)) ? 1 : 0;
        }

        /* (3) After-state check */
        trxvu_sleep_ms(DEV001_SETTLE_MS);
        (void)trxvu_cmd_trs_get_state(ctx, &state_after);

        /* (4) 결과 출력 */
        printf("\nResults:\n");
        printf(" Combo                      | Set CC   | Get CC   | sent       "
               "| read       | status | set rc     | get rc     | verdict\n");
        printf(" --------------------------------------------------------------"
               "----------------------------------------------------------------\n");
        for (i = 0; i < 4; i++)
        {
            dev001_print_result(&combos[i], i);
        }

        printf("\nTRS state before=0x%02X, after=0x%02X (delta=%s)\n",
               (unsigned)state_before, (unsigned)state_after,
               (state_before == state_after) ? "none" : "changed (review)");

        /* ICD-0002 §5.1.1.1: nominal 복귀 시 HW reset 동반 → 자동 X.
         * 사용자가 [50] Set TX mode 메뉴로 직접 nominal 전환 권장. */
        printf("\nNOTE: TX mode currently TRANSPONDER. To return to NOMINAL,\n");
        printf("      use menu [50] Set TX mode (mode=1). This triggers HW reset (ICD-0002 §5.1.1.1).\n");

        /* (5) 판정 로직 */
        printf("\n--- Verdict ---\n");
        if (combos[0].match && combos[3].match && !combos[1].match && !combos[2].match)
        {
            printf(" DIAGONAL MATCH (A+D) — 0x32/0x33 and 0x80/0x81 are SEPARATE commands.\n");
            printf(" → Both code pairs work but address different semantics.\n");
            printf(" → DEV-001 default (0x32/0x33) retained. Use _ALT for legacy access.\n");
        }
        else if (combos[0].match && combos[1].match && combos[2].match && combos[3].match)
        {
            printf(" ALL MATCH — 0x32/0x33 and 0x80/0x81 are SYNONYMS.\n");
            printf(" → DEV-001 default (0x32/0x33) retained.\n");
        }
        else if (!combos[0].match && combos[3].match)
        {
            printf(" LEGACY MATCH (D only) — 0x80/0x81 is the correct code.\n");
            printf(" → ACTION: swap default macros (0x80/0x81 → default, 0x32/0x33 → ALT).\n");
            printf(" → Update RULE_MATRIX DEV-001 status.\n");
        }
        else if (combos[0].match && !combos[3].match)
        {
            printf(" DEFAULT MATCH (A only) — 0x32/0x33 is the correct code (current default).\n");
            printf(" → DEV-001 RESOLVED: keep 0x32/0x33 as default.\n");
        }
        else
        {
            printf(" AMBIGUOUS result. Manual review required:\n");
            printf("   - Check firmware version (0x42)\n");
            printf("   - Verify VHF antenna connection (no DC short)\n");
            printf("   - Try with TX mode = transponder (0x38 0x02) first\n");
            result = -2;
        }
    }

    printf("============================================================\n\n");
    return result;
}
