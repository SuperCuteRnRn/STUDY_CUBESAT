# ITSDL_TRXVU flat API 레퍼런스 (Rev 1.0, 2026-07-14)

통신보드 **ITSDL_TRXVU** 를 open/close·context 없이 `TXU_*` 함수만으로 제어하는
flat API 문서. 상대방은 소스 없이 `txu.h` + `libtxu.a`/`.so` 만으로 보드를 제어할 수 있다.

- slave: **RCV 0x60 (Receiver) / TRS 0x61 (Transmitter)**
- I2C 디바이스 기본값: `/dev/i2c-1`
- 저수준 프로토콜 근거: ISIS TRXVU **ICD-0001**(공통/RX/TX 26 명령), **ICD-0002**(Transponder 5 명령)

---

## 1. 배포물

| 파일 | 설명 |
|---|---|
| `txu.h` | 공개 헤더 (함수 선언 + slave/mode/bitrate 상수). `<stdint.h>`/`<stddef.h>`만 의존 |
| `libtxu.a` | 정적 라이브러리 (`make lib`) |
| `libtxu.so` | 공유 라이브러리 (`make shared`) — Python ctypes 용 |
| `DOC_TXU_API_20260714.md` | 본 문서 |

> `.a`/`.so` 는 타겟(라즈베리파이/ARM)에서 `make dist` 실행 후 `dist/` 에 생성된다.

## 2. C 링크 예제

```c
#include "txu.h"
#include <stdio.h>

int main(void)
{
    uint8_t st;
    if (TXU_TrsGetState(&st) == 0) { printf("state=0x%02X\n", st); }
    TXU_Close();
    return 0;
}
```
```bash
gcc myapp.c -L. -ltxu -o myapp     # 정적
# 또는 공유: gcc myapp.c -L. -ltxu -Wl,-rpath,. -o myapp
```

## 3. Python (ctypes) 예제

```python
import ctypes
lib = ctypes.CDLL("./libtxu.so")

lib.TXU_GetUptime.restype  = ctypes.c_uint32
lib.TXU_GetUptime.argtypes = [ctypes.c_int]     # TXU_Slave (TXU_TRS = 1)

up = lib.TXU_GetUptime(1)
print("uptime =", up, "s" if up != 0xFFFFFFFF else "(ERROR)")

st = ctypes.c_uint8()
if lib.TXU_TrsGetState(ctypes.byref(st)) == 0:
    print("state = 0x%02X" % st.value)

lib.TXU_Close()
```

## 4. 반환 규약

| 종류 | 규약 |
|---|---|
| 명령형 | `0` 성공 / `-1` 오류 |
| 스칼라 조회 | 값 직접 반환, 오류 `0xFFFF`(`TXU_ERR_U16`) 또는 `0xFFFFFFFF`(`TXU_ERR_U32`) |
| 텔레메트리 (`TXU_RcvGetTelemetry`, `TXU_TrsGetTelemetry` …) | 반환값 = 읽은 byte 수(>0) / `-1` |
| 버퍼·다중값 | out 파라미터, 반환 `0/-1` |

I2C open 은 첫 명령에서 자동(lazy). 실패 시 각 함수는 오류값을 반환한다.

## 5. 함수표

### 5.1 라이프사이클
| 함수 | 인자 | 반환 | 설명 |
|---|---|---|---|
| `TXU_SetDevice` | `const char *path` | void | 첫 open 전에 I2C 경로 지정 |
| `TXU_Open` | — | `0/-1` | 명시적 open (생략 가능) |
| `TXU_Close` | — | void | 명시적 close |

### 5.2 공통 (RCV/TRS) — `slave` = `TXU_RCV`(0) 또는 `TXU_TRS`(1)
| 함수 | 인자 | 반환 | raw CC |
|---|---|---|---|
| `TXU_GetUptime` | `slave` | `uint32` s / `0xFFFFFFFF` | 0x40 |
| `TXU_GetFrequency` | `slave` | `uint32` kHz / `0xFFFFFFFF` | 0x33 |
| `TXU_SetFrequency` (!) | `slave, uint32 khz` | `0/-1` | 0x32 |
| `TXU_GetPllError` | `slave, u16* lock, u16* freq` | `0/-1` | 0x34 |
| `TXU_GetFirmwareInfo` | `slave, char[80]` | `0/-1` | 0x42 |
| `TXU_GetLastResetCause` | `slave, u8*` | `0/-1` | 0x50 |

### 5.3 RX (Receiver 0x60)
| 함수 | 인자 | 반환 | raw CC |
|---|---|---|---|
| `TXU_RcvGetTelemetry` | `u8[16]` | byte수/-1 | 0x1A |
| `TXU_RcvGetFrameCount` | `u16*` | `0/-1` | 0x21 |
| `TXU_RcvGetFrameLength` | `u16*` | `0/-1` | 0x25 |
| `TXU_RcvGetFrame` | `u8*, size_t max, size_t* act` | `0/-1` | 0x22 |
| `TXU_RcvRemoveFrame` | — | `0/-1` | 0x24 |
| `TXU_RcvRemoveAllFrames` | — | `0/-1` | 0x26 |

### 5.4 TX (Transmitter 0x61)
| 함수 | 인자 | 반환 | raw CC |
|---|---|---|---|
| `TXU_TrsGetTelemetry` | `u8[16]` | byte수/-1 | 0x25 |
| `TXU_TrsGetLastTxTelemetry` | `u8[16]` | byte수/-1 | 0x26 |
| `TXU_TrsGetState` | `u8*` | `0/-1` | 0x41 |
| `TXU_TrsSetIdle` | `u8 enable` | `0/-1` | 0x24 |
| `TXU_TrsSetBitrate` | `u8 mask` | `0/-1` | 0x28 |
| `TXU_TrsSetPllPower` | `u16` | `0/-1` | 0x35 |
| `TXU_TrsGetPaOverTemp` | `u8*` | `0/-1` | 0x60 |
| `TXU_TrsClearPaOverTemp` | — | `0/-1` | 0x61 |
| `TXU_TrsClearBeacon` | — | `0/-1` | 0x1F |
| `TXU_TrsSendFrame` (!) | `const u8*, u8 len, u8* rem` | `0/-1` | 0x10 |
| `TXU_TrsSetBeacon` (!) | `u16 s, const u8*, u8 len` | `0/-1` | 0x14 |
| `TXU_TrsGetToCallsign` | `u8[7]` | `0/-1` | 0x20 |
| `TXU_TrsGetFromCallsign` | `u8[7]` | `0/-1` | 0x21 |
| `TXU_TrsSetToCallsign` (!) | `const u8[7]` | `0/-1` | 0x22 |
| `TXU_TrsSetFromCallsign` (!) | `const u8[7]` | `0/-1` | 0x23 |
| `TXU_TrsSendFrameOverride` (!) | `to[7], from[7], frame, len, rem` | `0/-1` | 0x11 |
| `TXU_TrsSetBeaconOverride` (!) | `s, to[7], from[7], frame, len` | `0/-1` | 0x15 |

### 5.5 Transponder (ICD-0002, FM 전용)
| 함수 | 인자 | 반환 | raw CC |
|---|---|---|---|
| `TXU_SetTxMode` | `u8 mode` (`TXU_TX_MODE_*`) | `0/-1` | 0x38 |
| `TXU_GetTransponderRssi` | `u16*` | `0/-1` | 0x51 |
| `TXU_SetTransponderRssiThreshold` | `u16` (≤4095) | `0/-1` | 0x52 |
| `TXU_SetTransponderFrequency` (!) | `u32 khz` | `0/-1` | 0x32 |
| `TXU_GetTransponderFrequency` | `u32* khz, u8* status` | `0/-1` | 0x33 |

### 5.6 Reset / 시험
| 함수 | 인자 | 반환 | raw CC |
|---|---|---|---|
| `TXU_WatchdogReset` (!) | `slave` | `0/-1` | 0xCC |
| `TXU_SoftwareReset` (!) | `slave` | `0/-1` | 0xAA |
| `TXU_HardwareReset` (!) | `slave` | `0/-1` | 0xAB |
| `TXU_RunDev001Test` | — | `0/-1/-2` | (0x32/0x33 vs 0x80/0x81 4조합 비교) |

## 6. 위험 등급 표

| 등급 | 명령 | 처리 권고 |
|---|---|---|
| HIGH | SendFrame/SendFrameOverride, SetBeacon/SetBeaconOverride, Watchdog/SW/HW Reset | RF 송출·보드 리셋. 운용 규제·안전 절차 확인 후 실행 |
| MEDIUM | SetFrequency, SetTransponderFrequency, SetTo/FromCallsign | 주파수·콜사인 영구 변경 |
| NONE | 그 외 조회 | read-only, 즉시 가능 |

> `(!)` 표시 함수는 위 위험 등급에 해당. `txu.h` 배너 주석에도 명시되어 있다.

## 7. 비고

- Transponder freq 코드는 DEV-001(2026-05-21)에서 운영 기본 `0x32/0x33`으로 확정.
  `TXU_RunDev001Test()` 는 `0x32/0x33` vs `0x80/0x81` 4조합을 wire-level 비교한다.
- EM 보드에서는 Transponder 입력 freq 명령이 미구현(`status 0xFF`)이거나 nominal
  TX freq 명령으로 동작할 수 있다.
