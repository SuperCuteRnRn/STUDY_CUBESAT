# PLAN_TX_RAW_STRIP_20260716 — 위치 독립 raw RGB 스트립 전송

## 1. 목적
JPEG는 조각 하나만으론 못 그려서 손실 시 아무것도 안 뜬다. **비압축 RGB를 오프셋
스트립으로** 보내 지상국이 도착 조각을 즉시 제자리에 칠하게 한다(순서·손실 무관).
해상도 64×64 컬러(사용자 선택).

## 2. 포맷 (프레임 payload, TRXVU 255B 이내)
```
'IMG'(3) + img_id(1) + W(1) + H(1) + off(2 BE) + <raw RGB ≤200B>
```
- off = raw RGB 버퍼(W*H*3=12288B) 바이트 오프셋. 헤더 8B 고정(기존과 동일).

## 3. 변경 (rx_img_tx.c)
1. 상수: `IMG_PATH "img1.jpg"→"img.rgb"`, `IMG_W 64U`, `IMG_H 64U` 추가.
2. `capture_image()` rpicam 인자: `--width/height 320→64`, `--encoding rgb` 추가,
   출력 `img.rgb`.
3. `send_image()`: JPEG seq/total 청킹 → **raw off 스트립**으로 재작성
   (frame[4]=W, [5]=H, [6..7]=off BE). `send_one` 플로우 제어·`IMG_TX_REPEAT` 반복
   그대로 재사용.

## 4. MISRA-C:2012
- 단일 출구 유지, 매직넘버 `#define`(IMG_W/H), 변수 초기화, 명시적 캐스트,
  동적할당 없음(static frame 버퍼).

## 5. 리스크/가정
- 가정: `rpicam-still --encoding rgb` 가 packed RGB888 = W*H*3 바이트 출력.
  (스트라이드 패딩/ BGR 순서면 RX에서 포맷만 교체.)
- 64×64×3=12288B → 청크 200B 기준 ~62프레임. off<65535 이라 2바이트로 충분.
- 반복(IMG_TX_REPEAT)·플로우 제어(send_one)·idle 로직 불변.

## 6. 완료 기준
- `make` 통과. 콘솔 `전송 완료 (raw 64x64, 12288 B)`.
- 지상국이 도착 스트립을 즉시 표시(빠진 곳 회색).
