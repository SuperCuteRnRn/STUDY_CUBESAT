# ITSDL_EPS&BAT 라이브러리 API 레퍼런스

| 문서 번호 | ITSDL-API-EPSBAT-001 |
|---|---|
| **개정** | Rev. 1.2 |
| **발행일** | 2026-07-12 |
| **대상** | libepsbat 라이브러리 사용자 (Linux, ARM) |

교육용 큐브위성 시스템을 위한 전원 관리 보드 **ITSDL_EPS&BAT**를
사용자 프로그램에서 제어하기 위한 라이브러리 문서이다.
보드 사양·프로토콜 상세는 데이터시트(ITSDL-DS-EPSBAT-001)를 참조한다.

---

## 1. 배포물 구성

| 파일 | 용도 |
|---|---|
| `libepsbat.a` | 정적 라이브러리 (C 프로그램에 포함되어 빌드됨) |
| `libepsbat.so` | 공유 라이브러리 (실행 시 로드, Python ctypes 사용 가능) |
| `epsbat.h` | 공개 헤더 (함수 선언 + TLE 코드 상수) |
| 본 문서 | API 레퍼런스 |

요구 사항: Linux, I2C 버스 1(`/dev/i2c-1`) 활성화, i2c 접근 권한(root 또는 i2c 그룹).

## 2. C에서 사용하기

```c
/* example.c — 배터리 전압을 읽고 조건에 따라 PDM 채널 3을 제어 */
#include <stdio.h>
#include "epsbat.h"

int main(void)
{
    /* 1) 보드 확인 */
    if (EPS_GetVersion() != 0x0001) {
        printf("board not found\n");
        return 1;
    }

    /* 2) 배터리 전압 읽기 */
    uint16_t raw = EPS_GetTelemetry(EPS_TLE_VPCMBATV);
    if (raw == 0xFFFF) {
        printf("telemetry error (last error: 0x%04X)\n", EPS_GetLastError());
        return 1;
    }
    double vbat = raw * 0.008978;
    printf("VBAT = %.2f V\n", vbat);

    /* 3) 전압이 충분하면 BAT PDM(채널 3) ON */
    if (vbat > 7.0) {
        EPS_SwitchNPDMOn(3);
        printf("PDM3 state = %u\n", EPS_GetActualStateOfNPDM(3));
    }
    return 0;
}
```

빌드 (라이브러리·헤더가 현재 폴더에 있을 때) — **정적 링크 권장**:

```bash
# 방법 1 (권장): 정적 링크 — 실행 파일에 라이브러리 포함, 어디서든 실행 가능
gcc example.c libepsbat.a -o example

# 방법 2: 공유 라이브러리 링크 — 실행 파일 옆의 libepsbat.so를 자동으로 찾도록 rpath 지정
gcc example.c -L. -lepsbat -Wl,-rpath,'$ORIGIN' -o example

sudo ./example
```

> 참고: `-L. -lepsbat`만 사용하면 `.a`와 `.so`가 함께 있을 때 `.so`가 우선
> 링크되는데, 리눅스 로더는 실행 시 현재 폴더를 검색하지 않으므로
> `cannot open shared object file` 오류가 발생한다. 위 두 방법 중 하나를 사용한다.

## 3. Python에서 사용하기 (ctypes)

```python
import ctypes

eps = ctypes.CDLL("./libepsbat.so")
eps.EPS_GetTelemetry.restype = ctypes.c_uint16
eps.EPS_GetVersion.restype = ctypes.c_uint16

assert eps.EPS_GetVersion() == 0x0001

raw = eps.EPS_GetTelemetry(0xE220)          # BAT 버스 전압
if raw != 0xFFFF:
    print(f"VBAT = {raw * 0.008978:.2f} V")

eps.EPS_SwitchNPDMOn(3)                     # PDM 채널 3 ON
print("PDM3 =", eps.EPS_GetActualStateOfNPDM(3))
```

## 4. 공통 규약

- **반환값 오류 표시**: `uint16_t` 함수는 `0xFFFF`, `uint8_t` 채널 함수는 `0xFF`가
  오류(통신 실패, 잘못된 채널, busy)를 의미한다. 원인은 `EPS_GetLastError()`로 확인.
- **처리 시간**: 각 명령의 보드측 처리 시간은 라이브러리가 내부에서 자동 대기한다.
  호출자가 별도 딜레이를 넣을 필요 없다 (단, 호출 한 건당 수백 ms 소요될 수 있음).
- **PDM 지원 채널**: 1~6, 8, 9. 채널 매핑: 1/2=12V, 3/4=BAT, 5/6=5V, 8/9=3.3V.
- 스레드 안전하지 않다 — 한 프로세스에서 순차 호출한다.

## 5. API 레퍼런스

### 5.1 보드 상태/정보

| 함수 | 반환 | 설명 |
|---|---|---|
| `uint16_t EPS_GetBoardStatus(void)` | 상태 워드 | 정상 0x0000 |
| `uint16_t EPS_GetLastError(void)` | 오류 코드 | 0x01 명령 / 0x02 데이터 / 0x03 채널·TLE |
| `uint16_t EPS_GetVersion(void)` | 0x0001 | 펌웨어 버전 |
| `uint16_t EPS_GetChecksum(void)` | 0xA5A5 | 펌웨어 체크섬 |
| `uint16_t EPS_GetRevision(void)` | 0x0001 | 펌웨어 리비전 |

### 5.2 텔레메트리

| 함수 | 설명 |
|---|---|
| `uint16_t EPS_GetTelemetry(uint16_t TLECode)` | TLE 코드의 측정값(ADC count, 0~1023) 반환 |

**TLE 코드와 변환 공식** — 물리량 = 반환값 × 스케일

| 코드 (상수) | 항목 | 스케일 |
|---|---|---|
| 0xE220 `EPS_TLE_VPCMBATV` | BAT 버스 전압 | 0.008978 V |
| 0xE224 `EPS_TLE_IPCMBATV` | BAT 버스 전류¹ | 0.00681989 A |
| 0xE230 `EPS_TLE_VPCM12V` | 12V 버스 전압 | 0.01349 V |
| 0xE234 `EPS_TLE_IPCM12V` | 12V 버스 전류 | 0.00206663 A |
| 0xE210 `EPS_TLE_VPCM5V` | 5V 버스 전압 | 0.005865 V |
| 0xE214 `EPS_TLE_IPCM5V` | 5V 버스 전류 | 0.00681989 A |
| 0xE200 `EPS_TLE_VPCM3V3` | 3.3V 버스 전압 | 0.004311 V |
| 0xE204 `EPS_TLE_IPCM3V3` | 3.3V 버스 전류 | 0.00681989 A |
| 0xE308 `EPS_TLE_TBRD` | 보드 온도 1 | 0.372434×값 − 273.15 °C |
| 0xE388 `EPS_TLE_TBRD_DB` | 보드 온도 2 | 0.372434×값 − 273.15 °C |

¹ 배터리 전류는 방전 방향을 계측한다 (충전 중에는 0).

**PDM 채널 텔레메트리 (지원 채널: 1~6, 8, 9)** — 전압 코드 = `EPS_TLE_VSW1` + 16×(채널−1), 전류 = 전압 코드 + 4

| 채널 | 전압 스케일 (V) | 전류 스케일 (A) |
|---|---|---|
| 1, 2 (12V) | 0.01349 | 0.001328 |
| 3, 4 (BAT) | 0.008993 | 0.006239 |
| 5, 6 (5V) | 0.005865 | 0.001328 |
| 8, 9 (3.3V) | 0.004311 | 0.001328 |

### 5.3 리셋 카운터

| 함수 | 항목 |
|---|---|
| `uint16_t EPS_GetNumberOfBrownOutReset(void)` | 워치독 리셋 횟수 |
| `uint16_t EPS_GetNumberOfAutomaticReset(void)` | SW 리셋 횟수 |
| `uint16_t EPS_GetNumberOfManuelReset(void)` | 핀 리셋 횟수 |
| `uint16_t EPS_GetNumberOfCommWatchdogReset(void)` | 전원 투입 횟수 |

카운터는 보드 전원 인가 중 유지된다.

### 5.4 PDM (부하 분배) 제어

**지원 채널: 1~6, 8, 9 (총 8채널)** — 아래 함수의 `ch` 인자에 사용한다.

| 채널 | 1 | 2 | 3 | 4 | 5 | 6 | 8 | 9 |
|---|---|---|---|---|---|---|---|---|
| 레일 | 12V | 12V | BAT | BAT | 5V | 5V | 3.3V | 3.3V |

지원 범위 밖의 채널은 함수가 0xFF/0xFFFF를 반환한다 (보드 전송 없음).

| 함수 | 반환 | 설명 |
|---|---|---|
| `uint8_t EPS_IsValidPDMChannel(uint8_t ch)` | 1/0 | 지원 채널 여부 (통신 없음) |
| `uint8_t EPS_SwitchNPDMOn(uint8_t ch)` | 0=성공, 0xFF=채널 오류 | 채널 ON |
| `uint8_t EPS_SwitchNPDMOff(uint8_t ch)` | 〃 | 채널 OFF |
| `uint8_t EPS_SwitchAllPDMOn(void)` | 0=성공 | 전 채널 ON |
| `uint8_t EPS_SwitchAllPDMOff(void)` | 0=성공 | 전 채널 OFF |
| `uint16_t EPS_GetActualStateOfNPDM(uint8_t ch)` | 0/1 | 채널 현재 상태 |
| `uint16_t EPS_GetActualStateOfAllPDM(void)` | 비트맵 | 현재 상태 (bit0=채널1) |
| `uint16_t EPS_GetExpectedStateOfAllPDM(void)` | 비트맵 | 기대 상태 |
| `uint16_t EPS_GetInitialStateOfAllPDM(void)` | 비트맵 | 부팅 초기 상태 설정값 |
| `uint8_t EPS_SetInitialStateOfNPDMOn(uint8_t ch)` | 0=성공 | 부팅 초기상태 ON 설정 |
| `uint8_t EPS_SetInitialStateOfNPDMOff(uint8_t ch)` | 〃 | 부팅 초기상태 OFF 설정 |
| `uint8_t EPS_SetAllPDMToInitialState(void)` | 0=성공 | 전 채널을 초기 설정값으로 |

비트맵 해석 예: 반환값 `0x0005` → bit0, bit2 세트 → 채널 1, 3이 ON.

---

## 6. 문의

기능 추가·오류 문의는 보드 공급자에게 연락한다.
