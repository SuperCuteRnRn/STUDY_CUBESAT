# PLAN_TX_REPEAT_20260716 — 이미지 반복 전송(손실 구멍 채우기)

## 1. 작업 목적 및 배경
지상국 실측: 105조각 중 14조각만 디코드(약 87% 손실), 게다가 흩어져 도착(seq 0~10
유실). 한 번 전송으론 이미지 완성이 불가. **같은 img_id 로 이미지를 여러 번 반복
전송**하면 지상국 재조립이 seq별로 저장하므로 회차마다 다른 구멍이 채워져 점차
완성된다(지상국 RX는 이미 seq 딕셔너리로 누적 → 코드 변경 불필요).

## 2. 작업 범위
- 수정 파일: `rx_img_tx.c` (1개)
- 변경 지점:
  1. 상수 추가: `IMG_TX_REPEAT`
  2. `do_capture_and_send()` — 읽은 이미지를 같은 img_id 로 `IMG_TX_REPEAT` 회 반복 전송

## 3. 구현 방법 (변경 전/후 코드)

### 3.1 상수 추가 (타이밍/기타 블록, `TX_RETRY_MAX` 부근)
```c
#define IMG_TX_REPEAT     3U          /* 손실 대비 이미지 반복 전송 횟수 */
```

### 3.2 `do_capture_and_send()` 반복 루프 (339~364행)
변경 전:
```c
        long n = read_file(IMG_PATH, jpeg_buf, IMG_BUF_MAX);

        if (n > 0)
        {
            if (send_image(jpeg_buf, (size_t)n, img_id) == 0)
            {
                sent_ok = 1;
            }
        }
        else
        {
            printf("[ERR] 이미지 읽기 실패\n");
        }
```
변경 후:
```c
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
```
- 같은 `img_id`·같은 데이터로 3회 반복. `sent_ok` 은 한 번이라도 성공하면 1
  → `main` 에서 img_id 1회 증가(다음 이미지는 새 id).
- `g_running` 검사로 Ctrl-C 시 반복 중단.

## 4. MISRA-C:2012 준수
- 단일 출구(함수 `return sent_ok` 1개) 유지.
- 매직넘버 금지: 반복 횟수 `IMG_TX_REPEAT` `#define`.
- 반복 변수 `pass` 초기화, `for` 경계 명확.

## 5. 예상 영향 범위 및 리스크
- 전송 시간: 이미지 1장당 (30s × 3) ≈ 90초. 신뢰성과의 트레이드오프.
- ⚠️ **한계**: 프레임 손실률이 ~87%로 매우 높아 3회 반복으로도 완전 완성이 안 될 수
  있음(커버리지 ≈ 1-0.87³ ≈ 34%). 근본적으로는 **손실률 자체를 낮춰야**(TX 페이싱을
  더 늘려 수신기 재동기 시간 확보 등) 반복이 효과적. → 후속 과제로 별도 조사.
  `IMG_TX_REPEAT` 값은 필요시 상향 가능.
- `send_image`/`send_one` 로직 불변.

## 6. 완료 기준
- `make` 통과.
- (기능) 한 트리거에 대해 콘솔에 `반복 1/3, 2/3, 3/3` 출력, 지상국 누적 수신 조각
  수가 회차마다 증가. RX partial 표시와 결합 시 이미지가 점차 채워짐.
