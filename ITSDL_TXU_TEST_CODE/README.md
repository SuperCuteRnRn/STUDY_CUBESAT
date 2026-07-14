# ITSDL_TRXVU 테스트 코드 (flat API)

교육용 큐브위성 시스템을 위한 통신보드 **ITSDL_TRXVU**의 I2C 드라이버 및 테스트
프로그램이다. Linux 기반 상위 시스템(OBC)에서 I2C 버스를 통해 보드를 제어한다.
외부 사용자는 **open/close·context 관리 없이 `TXU_*` 함수만 호출**하면 된다.

- 슬레이브 주소: **RCV 0x60 (Receiver) / TRS 0x61 (Transmitter)**
- I2C 디바이스: 기본 `/dev/i2c-1` (`TXU_SetDevice()`로 변경 가능)
- 상세 사양: `docs/` (API 레퍼런스) + ISIS TRXVU ICD-0001/0002 참조

## 파일 구성

| 파일 | 설명 |
|---|---|
| `txu_test.c` | **통합 테스트 프로그램** (메뉴 방식, 기본 빌드 대상) |
| `txu.c` / `txu.h` | **flat API** — context 은닉, 함수 호출만으로 제어 |
| `trxvu_commands.c` / `.h` | 명령 wrapper (ICD-0001 26 + ICD-0002 5 = 31 명령) — 내부 계층 |
| `trxvu_i2c.c` / `.h` | Linux i2c-dev 래퍼 — 내부 계층 |
| `trxvu_dev001.c` / `.h` | DEV-001 transponder freq 코드 자동 비교 시험 — 내부 계층 |

> 외부 배포 시 공개 헤더는 `txu.h` **하나**다. `trxvu_*` 는 내부 구현용.

## 빌드 및 실행

```bash
make               # txu_test 빌드
sudo ./txu_test    # 메뉴 방식 통합 테스트 (I2C 접근에 root 필요)

make lib shared    # libtxu.a / libtxu.so
make dist          # 배포 패키지 dist/ (헤더+라이브러리+문서)
make clean
```

> 위성 OBC 에서는 cFS 등 I2C 동시 사용 프로세스를 먼저 정지시킨다
> (`sudo pkill -f core-cpsat`).

`txu_test` 메뉴:
1)보드 정보  2)상태/텔레메트리  3)RX 프레임 큐  4)PLL 오류  5)AX.25 콜사인
6)Transponder RSSI  7)TX Idle  8)TX Bitrate  80)Reset(확인)  90)DEV-001 시험

## 사용 예 (C)

```c
#include "txu.h"
#include <stdio.h>

int main(void)
{
    uint8_t st;
    uint32_t up = TXU_GetUptime(TXU_TRS);   /* 내부에서 I2C 자동 open */
    if (up != TXU_ERR_U32) { printf("uptime=%u s\n", up); }

    if (TXU_TrsGetState(&st) == 0) { printf("TRS state=0x%02X\n", st); }

    TXU_Close();   /* 선택: 종료 시 정리 */
    return 0;
}
```
빌드: `gcc myapp.c -L. -ltxu -o myapp`

## 반환 규약

| 종류 | 규약 |
|---|---|
| 명령형 함수 | `0` = 성공, `-1` = 오류 |
| 스칼라 조회 (`TXU_GetUptime`, `TXU_GetFrequency`) | 값 직접 반환, 오류 = `0xFFFFFFFF` (`TXU_ERR_U32`) |
| 버퍼/다중값 (`TXU_*GetTelemetry`, `TXU_GetPllError` …) | out 파라미터, 반환 `0/-1` (텔레메트리는 읽은 byte 수) |

## 위험 명령 (호출 전 운용 규제·안전 절차 확인)

| 명령 | 위험 | 비고 |
|---|---|---|
| `TXU_TrsSendFrame` / `TXU_TrsSendFrameOverride` | HIGH | RF 송출 |
| `TXU_TrsSetBeacon` / `TXU_TrsSetBeaconOverride` | HIGH | RF 반복 송출 |
| `TXU_WatchdogReset` / `TXU_SoftwareReset` / `TXU_HardwareReset` | HIGH | 보드 리셋 |
| `TXU_SetFrequency` / `TXU_SetTransponderFrequency` | MEDIUM | 주파수 변경 |
| `TXU_TrsSetToCallsign` / `TXU_TrsSetFromCallsign` | MEDIUM | 콜사인 영구 변경 |
| 그 외 조회 | NONE | read-only |

> 메뉴(`txu_test`)에는 안전상 Reset(확인 프롬프트)만 노출한다. RF 송출·주파수 변경·
> 콜사인 영구 변경 명령은 `TXU_*` API 로 **코드에서 직접** 사용한다.

## 참고 사항

- I2C 장치 접근에는 root 권한 또는 i2c 그룹 권한이 필요하다.
- Transponder 명령(ICD-0002)은 **FM(Flight Model) 전용**이다. EM 보드에서는 미구현
  응답(`0xFF status`) 또는 nominal freq 명령으로 동작할 수 있다.
- 개발 PC(Windows)에서는 `linux/i2c-dev.h` 부재로 빌드 불가 — 타겟(라즈베리파이/ARM)에서 빌드.
