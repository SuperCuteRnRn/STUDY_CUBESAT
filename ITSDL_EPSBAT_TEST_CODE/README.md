# ITSDL_EPS&BAT 테스트 코드

교육용 큐브위성 시스템을 위한 전원 관리 보드 **ITSDL_EPS&BAT**의
I2C 드라이버 및 테스트 프로그램이다. Linux 기반 상위 시스템(OBC)에서
I2C 버스를 통해 보드를 제어하고 텔레메트리를 수집한다.

- 슬레이브 주소: **0x2B** (100 kHz)
- 상세 사양: `Implement/DOC_DATASHEET_ITSDL_BAT_20260712.md` (데이터시트) 참조

## 파일 구성

| 파일 | 설명 |
|---|---|
| `eps_test.c` | **통합 테스트 프로그램** (모든 기능을 메뉴로 제공, 기본 빌드 대상) |
| `function.c` / `function.h` | ITSDL_EPS&BAT 명령 라이브러리 |
| `i2c_lib.c` / `i2c_lib.h` | Linux I2C 디바이스 래퍼 |
| `main.c`, `repeat.c`, `for_main.c` 등 | 개별 테스트 프로그램 (`make legacy`) |

## 빌드 및 실행

```bash
make          # eps_test 빌드
./eps_test    # 메뉴 방식 통합 테스트

make legacy   # 기존 개별 프로그램 빌드 (선택)
```

`eps_test` 메뉴:
1)보드 정보  2)전체 텔레메트리  3)PCM 반복 모니터링  4/5)PDM 개별 ON/OFF
6/7)PDM 전체 ON/OFF  8)PDM 상태(Actual/Expected/Initial)  9)PDM 초기상태 설정
10)리셋 카운터

## 명령 요약

| 분류 | 명령 |
|---|---|
| 상태/정보 | Board Status(0x01), Last Error(0x03), Version(0x04)=0x0001, Checksum(0x05)=0xA5A5, Revision(0x06)=0x0001 |
| 텔레메트리(0x10) | 전원 버스 4종 전압/전류, PDM 채널 전압/전류, 보드 온도 2점 |
| 리셋 카운터 | 0x31(워치독), 0x32(SW), 0x33(핀), 0x34(전원 투입) |
| PDM | 전체 ON/OFF(0x40/41), 상태(0x42/43/44), 초기화(0x45/52/53), 개별 ON/OFF(0x50/51), 개별상태(0x54) |

- **PDM 지원 채널: 1/2=12V, 3/4=BAT, 5/6=5V, 8/9=3.3V** —
  라이브러리의 `EPS_IsValidPDMChannel()`이 채널 입력을 검증한다.
- 정의 범위 밖의 요청은 0xFFFF + Last Error로 응답된다.
- 명령별 처리 시간(busy)은 라이브러리가 딜레이 인자로 자동 준수한다
  (Get Telemetry 15 ms, Checksum 70 ms, PDM 초기상태 설정 200 ms 등).

## 응답 형식 요약

| 명령 | 응답 | 파싱 |
|---|---|---|
| 0x01 | `[0, Status, 0, 0]` | `(b1<<8)\|b3` |
| 0x03 / 0x04 / 0x31 / 0x33 | `[hi, lo, 0, 0]` | `(b0<<8)\|b1` |
| 0x05 / 0x06 | `[0, hi, 0, lo]` | `(b1<<8)\|b3` |
| 0x32 | `[0, cnt, 0, 0]` | `b1` |
| 0x10 / 0x34 / 0x54 | 2바이트 | `(b0<<8)\|b1` |
| 0x42/0x43/0x44 | `[0, 0, hi, lo]` | 비트맵 (`GetPDMStateFlag()`) |

## 참고 사항

- 배터리 전류 텔레메트리는 방전 방향을 계측한다 (충전 중 0xE224는 0).
- 리셋 카운터·PDM 초기상태는 전원 인가 중 유지된다.
- I2C 장치 접근에는 root 권한 또는 i2c 그룹 권한이 필요하다.

## 오류 코드 (i2c_lib)

| 코드 | 의미 |
|---|---|
| I2C_SUCCESS (0) | 성공 |
| I2C_ERR_CHANNEL (-2) | 잘못된 채널 |
| I2C_ERR_DEVICE (-3) | 디바이스 열기 실패 |
| I2C_ERR_SLAVE (-4) | 슬레이브 주소 설정 오류 |
| I2C_ERR_WRITE / READ (-5/-6) | 쓰기/읽기 오류 |
| I2C_ERR_WRITE_READ (-7) | 쓰기+읽기 오류 |
