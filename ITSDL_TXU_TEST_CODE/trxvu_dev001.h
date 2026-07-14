/* ============================================================================
 * trxvu_dev001.h — transponder 주파수 코드 자동 비교 시험 prototype
 * (PLAN_TRX_HW_TEST_STANDALONE_20260521.md, 2026-05-21)
 * ========================================================================== */
#ifndef TRXVU_DEV001_H
#define TRXVU_DEV001_H

#include "trxvu_i2c.h"

/**
 * @brief  transponder 주파수 코드 4-조합 자동 시험 + 판정 출력.
 *
 * @return 0 = 결정됨 (combo 결과 명확), -1 = baseline 실패, -2 = ambiguous.
 */
int trxvu_dev001_run(trxvu_ctx_t *ctx);

#endif /* TRXVU_DEV001_H */
