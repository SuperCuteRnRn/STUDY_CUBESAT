# PLAN_RX_IMG_TX_20260716 — 프리셋10 수신 시 촬영·이미지 조각 전송

## 1. 작업 목적 및 배경
지상국이 프리셋 10(ASCII `"10"`)을 송신하면, CubeSat(라즈베리파이 + ITSDL_TRXVU)이
`rpicam-still` 로 320×320 촬영 후 이미지를 조각내어 TRXVU 로 다운링크한다.
TRXVU 한 프레임은 최대 255B(`TXU_TrsSendFrame` 의 len 이 uint8_t)라, 320×1 JPEG 행
(≥705B)은 한 프레임에 못 들어간다 → **통짜 JPEG 조각화** 방식 사용.

## 2. 작업 범위
- 신규 폴더: `STUDY_CUBESAT/cmd_rx_img_tx/`
  - `rx_img_tx.c` — 메인 (RX 폴링 → 트리거 → 촬영 → 조각 전송)
  - `Makefile` — `../ITSDL_TXU_TEST_CODE/libtxu.a` 정적 링크
  - `Implement/PLAN_RX_IMG_TX_20260716.md` — 본 계획서
- 지상국(steamdeck) 수신부는 별도 계획(PLAN_RX_IMAGE_20260716 addendum)에서 동일 포맷으로 수정.

## 3. 온에어 포맷 (지상국과 합의)
AX.25 UI 프레임 info 필드:
```
'IMG'(3B) + img_id(1B) + seq(2B, big-endian) + total(2B, big-endian) + chunk(≤200B)
```
- 헤더 8B + chunk ≤200B = 프레임 ≤208B (255 한도 이내).
- img_id: 촬영마다 1씩 증가(uint8_t wrap). seq: 0..total-1. total: 전체 조각 수.
- 지상국은 같은 img_id 조각을 seq 순서로 모아 total 개 완성 시 이어붙여 JPEG 디코드.

## 4. 동작 흐름
1. SIGINT/SIGTERM 핸들러 등록(정상 종료).
2. 무한 루프:
   - `TXU_RcvGetFrameCount` 로 수신 프레임 유무 확인(없으면 0.2s 대기).
   - 있으면 `TXU_RcvGetFrame` 로 읽고 `TXU_RcvRemoveFrame` 로 제거.
   - `frame_is_trigger()` 가 프리셋10(`"10"`)이면:
     a. `capture_image()` — fork/execvp 로 rpicam-still 실행:
        `rpicam-still --nopreview --autofocus-mode auto --width 320 --height 320 -o img1.jpg`
     b. `read_file()` — POSIX open/read/close 로 img1.jpg 를 static 버퍼(256KB)에 로드.
     c. `send_image()` — **TXU_TrsSetIdle(1)** → 조각 전송 → **TXU_TrsSetIdle(0)**.
        (사용자 요구: TX 시 idle ON, 다 보내면 idle OFF)
3. 종료 시 `TXU_Close()`.

## 5. 트리거 판정(frame_is_trigger)
- 수신 payload 에 AX.25 UI 헤더(… 0x03 0xF0 info)가 있으면 info 부분만 취함(없으면 전체).
- 앞뒤 공백/CR/LF 제거 후 정확히 `"10"` 이면 트리거.
- 가정: 지상국은 프리셋10을 텍스트 "10" 으로 송신, TRXVU 는 info(또는 AX.25 프레임)를 전달.
  (보드가 doppler/rssi 등 메타를 앞에 붙이면 이 함수만 조정 — 실기 확인 필요.)

## 6. MISRA-C:2012 준수/예외 (사용자 승인 완료)
- 준수: 단일 출구(15.5), goto 없음(14.x), 변수 초기화, 명시적 캐스트(10.x), switch 없음,
  동적할당 없음(21.3, static 버퍼), 반환값 확인, 매직넘버는 #define.
- 예외(각 위반부에 `/* MISRA-C:2012 Rule X deviation: 사유 */` 주석):
  - **Rule 21.8**(`system()` 금지): 회피 — `fork()`+`execvp()`+`waitpid()` 사용(system 미사용).
  - **Rule 21.6**(`<stdio.h>` 입출력 금지): 로그 출력에 `printf` 사용(기존 txu_test.c 와 동일).
    파일 입출력은 POSIX `open/read/close` 로 회피.
  - **Rule 7.4**(문자열 리터럴→char*): execvp argv 는 static char[] 배열로 구성해 회피.

## 7. 주요 상수
| 이름 | 값 | 의미 |
|---|---|---|
| IMG_CHUNK_MAX | 200 | 프레임당 JPEG 조각 최대 |
| IMG_HDR_LEN | 8 | magic(3)+id(1)+seq(2)+total(2) |
| IMG_BUF_MAX | 262144 | 촬영 JPEG 최대(256KB) static |
| TRIGGER_CMD | "10" | 프리셋10 |
| RX_POLL_NS | 2e8 | RX 폴링 0.2s |
| TX_GAP_NS | 5e7 | 프레임 간 0.05s |

## 8. 리스크
- rpicam-still 미설치/카메라 미연결 시 촬영 실패 → 전송 생략, 로그.
- 조각 손실 시 지상국이 그 이미지 디코드 실패(재촬영/재전송 필요) — 알려진 트레이드오프.
- TX 버퍼 포화 시 재시도(최대 20회, 0.05s 간격). 초과 시 해당 이미지 전송 실패.
- 트리거 판정은 실제 수신 프레임 레이아웃 확인 후 미세조정 필요.

## 8-1. 후속 수정 (수신 EIO 강건화)
증상: RF 프레임 수신 시 `TXU_RcvGetFrame`(큰 read)에서 `write(wr-rd): I/O error`(EIO).
유휴 폴링(2B GetFrameCount)은 정상 → 큰 read 트랜잭션만 실패 = 파이 I2C 클럭
스트레칭 이슈 정황. 코드측 완화:
- `rcv_read_frame()`: `TXU_RcvGetFrameLength` 로 실제 길이를 먼저 구해 **그만큼만** 읽음
  (고정 256 과다 read 제거).
- `rcv_frame_count()`/`rcv_read_frame()`: 간헐 EIO 대비 **재시도**(RX_RETRY_MAX=3, 5ms 간격).
- 읽기 최종 실패 시 스팸 대신 `[WARN]` 1회 로그 후 프레임 제거하고 계속.
- 근본 우회책(코드로 안 되면): `/boot/firmware/config.txt` 에
  `dtparam=i2c_arm=on,i2c_arm_baudrate=10000` 로 I2C 속도 낮춤 → 재부팅.

## 8-2. 근본 수정 (I2C 계층 교체 — libtxu → 검증된 i2c_lib)
증상: RF 프레임 수신 시 libtxu `TXU_RcvGetFrame` 이 항상 EIO(`write(wr-rd)`).
원인: **libtxu 의 I2C 접근 방식 문제**(하드웨어 무관, i2cdetect 에 0x60/0x61 정상).
  - libtxu `trxvu_i2c.c`: fd 1개 유지, 슬레이브 선택 직후 **즉시 write** → 큰
    프레임 읽기 트랜잭션에서 EIO.
  - 검증된 `Manufacturing_data/i2c_lib.c`: **트랜잭션마다 open→2ms settle→
    write/read→close + 3단계 재시도** → 정상 동작(현장 검증됨).
추가 버그: `0x22 Get Frame` 응답 앞 **6B 헤더(frame_size/doppler/rssi)** 를
  libtxu 사용 시 무시 → 트리거 파싱 불가. `trx_rcv_function.c` 의
  `TRX_RCV_Get_Frame` 이 헤더를 벗겨 순수 payload(`frame_contents`)만 제공.
조치:
- 벤더 파일 `i2c_lib.[ch]`, `trx_rcv_function.[ch]` 를 `cmd_rx_img_tx/` 에 복사.
- 수신: `TRX_RCV_Get_Frame_num`(0x60)/`TRX_RCV_Get_Frame`(0x60) 사용,
  `frame_contents` 로 `"10"` 판정.
- 송신: 같은 i2c_lib 로 TRS(0x61) `I2C_WriteReadBytes(cmd 0x10, ...)`,
  idle 은 `I2C_WriteBytes(cmd 0x24, enable)`.
- Makefile: 벤더 2개 .c 함께 컴파일, `-lpthread`. libtxu 링크 제거.
검증: `-Wall -Wextra` 경고 0 빌드, 시작·I2C 초기화·시그널 종료 정상(개발 PC).
벤더 파일은 원본 그대로 사용(별도 MISRA 준수 대상 아님).

## 9. 완료 기준
- `make` 로 경고 없이 빌드(`-Wall -Wextra`).
- (기능) 프리셋10 수신 → img1.jpg 생성 → 조각 전송 로그(idle ON→OFF) 확인.
- 지상국에서 완성 이미지 표시.
- 실제 하드웨어(카메라 + TRXVU) 검증은 사용자 환경 몫.
