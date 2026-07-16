# PLAN_TX_FLOWCTRL_20260716 — TX 버퍼 플로우 제어 (조각 유실 수정)

## 1. 작업 목적 및 배경
증상: 이미지 전송 시 지상국에 **조각 3~4개만 도착**. TX 콘솔은
`[OK] 이미지 id=2 전송 완료 (170 조각, 33882 B)` 로 **전량 전송 성공**으로 표시.

원인(확정): `send_one()`(`rx_img_tx.c:246`)이 TRXVU `TXU_TrsSendFrame`(0x10)의
응답 바이트 `rem`(남은 TX 슬롯 수)을 **읽기만 하고 검사하지 않는다.** I2C 읽기가
1바이트 성공이면 무조건 성공 처리한다.

TRXVU TX 버퍼는 슬롯이 몇 개뿐인데, TX는 50ms(`TX_GAP_NS`)마다 프레임을 밀어넣고
9600bps 무선 전송은 프레임당 수백 ms 걸린다. → 버퍼가 3~4프레임 만에 포화되고,
이후 프레임은 `rem==0xFF`(버퍼 풀, 프레임 거부)로 반환되지만 코드가 성공으로
착각해 `off/seq`를 전진 → **나머지 조각 전부 유실.**

설계 의도(`PLAN_RX_IMG_TX_20260716.md:66`)는 "TX 버퍼 포화 시 재시도"였으나 코드에
미구현 상태. 본 작업은 그 플로우 제어를 실제로 구현한다.

## 2. 작업 범위
- 수정 파일: `rx_img_tx.c` (1개)
- 변경 지점:
  1. 상수 추가: `TRS_TX_BUF_FULL`(0xFF), `TX_FULL_WAIT_NS`(버퍼 풀 대기)
  2. `send_one()` — `rem` 검사 기반 백프레셔로 재작성

## 3. 구현 방법 (변경 전/후 코드)

### 3.1 상수 추가 (타이밍 블록, 65행 `TX_RETRY_MAX` 아래)
변경 후:
```c
#define TX_RETRY_MAX      20U
#define TRS_TX_BUF_FULL   0xFFU        /* 0x10 응답: 버퍼 포화(프레임 미수록) */
#define TX_FULL_WAIT_NS   200000000L   /* 버퍼 풀 시 슬롯 대기(0.2s) */
```

### 3.2 `send_one()` 재작성 (246~267행)
변경 전:
```c
static int send_one(uint8_t *frame, uint8_t flen)
{
    int     result = -1;
    uint8_t tries  = 0U;

    for (tries = 0U; (tries < TX_RETRY_MAX) && (result != 0); tries++)
    {
        uint8_t rem = 0U;

        /* write [0x10 | frame], read 1 byte(remaining slots) */
        if (I2C_WriteReadBytes(I2C_CH, TRS_ADDR, CC_TRS_SEND_FRAME,
                               flen, frame, 1U, &rem, TX_RD_DELAY_MS) == 1)
        {
            result = 0;
        }
        else
        {
            sleep_ns(TX_GAP_NS);
        }
    }
    return result;
}
```
변경 후:
```c
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
```
- 핵심: `rem != TRS_TX_BUF_FULL` 일 때만 성공. 버퍼 풀이면 성공 처리하지 않으므로
  `send_image()`의 `off/seq` 전진이 일어나지 않고 **같은 프레임을 다시 보낸다.**
- 20회 재시도 × 0.2s = 최대 4s 대기/프레임. 한 프레임 공중 전송(<1s) 안에 슬롯이
  최소 1개는 비므로 정상 상황에선 1~3회 재시도로 수록된다. 그래도 계속 풀이면
  기존 설계대로 실패(-1) → 해당 이미지 전송 중단.

## 4. MISRA-C:2012 준수
- Rule 15.5 (단일 출구): 함수 `return` 1개 유지.
- 매직 넘버 금지: `0xFF`→`TRS_TX_BUF_FULL`, 대기시간→`TX_FULL_WAIT_NS` 로 `#define`.
- 모든 변수 선언 시 초기화(`result`, `tries`, `rem`).
- 함수 반환값(`I2C_WriteReadBytes`) 확인.
- 제어 흐름 명확(goto/재귀 없음), 암묵적 변환 없음.

## 5. 예상 영향 범위 및 리스크
- 정상화: 버퍼 포화 시 대기·재시도하므로 170조각 전부 실제 전송된다.
- 전송 시간 증가: 9600bps + 프레임당 대기로 170조각 전송에 수십 초 소요(정상 동작).
  (후속 최적화 옵션: JPEG 품질/해상도를 낮춰 조각 수 감소 — 별건, 이번 범위 아님.)
- **가정**: `rem==0xFF`가 "버퍼 풀"이라는 ISIS TRXVU 표준 규격. 저장소에 매뉴얼이
  없어 실기 검증 필요. 값이 다르면 `TRS_TX_BUF_FULL` 정의만 교체하면 됨.
- `send_one` 외 로직(`send_image`/`do_capture_and_send`) 불변.

## 6. 완료 기준
- 컴파일 통과(`make`).
- (기능) 이미지 전송 시 지상국이 **total 개 조각을 모두 수신**하고 이미지가 완성 표시.
- TX 콘솔 `전송 완료 (N 조각)`의 N 과 지상국 수신 조각 수 일치.
- HW/RF 실기 검증은 사용자 환경에서 확인.
