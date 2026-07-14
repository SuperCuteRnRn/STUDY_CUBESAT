# EPSBAT 텔레메트리 비콘 예제 (eps_tlm_beacon)

EPSBAT 전원보드의 **전체 텔레메트리**를 **1초 목표 주기**로 읽어, 통신보드(TXU)를 통해
AX.25 프레임으로 RF 송출하는 예제이다.

- **To (지상국)** : `ITSDGS`, SSID `0`
- **From (위성)** : `ITSSAT`, SSID `0`
- 콜사인은 `TXU_TrsSendFrameOverride()` 로 **프레임마다 지정**한다(보드 저장 콜사인 영구 변경 없음).
- **공개 API 레퍼런스만 사용**: `epsbat.h`(EPS_*) + `txu.h`(TXU_*). 내부 계층(`trxvu_*`)·raw I2C 미사용.

## 파일

| 파일 | 설명 |
|---|---|
| `eps_beacon.c` | C 예제 (MISRA-C:2012 준수). 기본 `TX_ENABLE 1`(실제 송출) |
| `eps_beacon.py` | Python(ctypes) 예제. 기본 `TX_ENABLE True`(실제 송출) |
| `Makefile` | libepsbat + libtxu 동시 링크 |

## 전송 항목 (한 사이클)

`KEY=VALUE` 공백 구분 ASCII. 200 byte 초과 시 자동으로 여러 프레임 분할(동일 To/From).

- 헤더/상태: `SEQ`, `ST`(상태워드)
- 전원 버스 8종: `VB IB V12 I12 V5 I5 V3 I3` (물리값 V/A)
- 보드 온도 2종: `T1 T2` (°C)
- PDM 채널(1~6,8,9) 각 V/I: `P1V P1I … P9V P9I`
- 리셋 카운터 4종: `BOR`(워치독) `AR`(SW) `MR`(핀) `CWR`(전원투입)

예)
```
ITSSAT TLM SEQ=12 ST=0x0000 VB=7.85 IB=0.34 V12=12.10 I12=0.05 V5=5.01 I5=0.20
V3=3.31 I3=0.15 T1=23.4 T2=24.1 P1V=12.09 P1I=0.02 ... P9V=3.30 P9I=0.01
BOR=0 AR=1 MR=0 CWR=2
```

## 빌드 (타겟: 라즈베리파이/ARM)

라이브러리 `.a`/`.so` 는 각 배포 패키지에서 `make dist` 후 생성된다. 경로가 다르면 인자로 지정.

```bash
# 정적 링크(권장)
make
# 경로 지정 예:
make EPS_DIR=/path/to/epsbat/library TXU_DIR=/path/to/ITSDL_TXU_TEST_CODE
# 공유 링크 대안:
make shared
```

> 개발 PC(Windows)에서는 `linux/i2c-dev.h` 부재로 빌드 불가 — 타겟에서 빌드/실행.

## 실행

```bash
sudo ./eps_beacon        # I2C 접근에 root 권한 필요
# Python:
sudo python3 eps_beacon.py
```

Ctrl-C 로 안전 종료(`TXU_Close()` 호출).

## ⚠ 안전 (필독)

- `TXU_TrsSendFrameOverride` 는 **실제 RF 를 송출**한다(HIGH RISK). 기본값이 **송출 ON**이다.
  주파수 허가·안테나 정합·운용 규제를 확인한 환경에서만 실행하라.
- **검증 단계에서는 dry-run 권장**:
  - C: `eps_beacon.c` 의 `#define TX_ENABLE (0)` 로 빌드 → 송출 없이 프레임만 출력.
  - Python: `eps_beacon.py` 의 `TX_ENABLE = False`.
- 위성 OBC 에서는 I2C 를 동시에 쓰는 프로세스를 먼저 정지(`sudo pkill -f core-cpsat`).

## 참고 / 한계

- **주기**: API 레퍼런스가 "EPS 호출 한 건당 수백 ms 소요 가능"이라 명시한다. 전체(~30회)
  조회가 1초를 넘으면 실측 주기는 1초보다 길어진다(back-to-back). **1초는 목표(최소) 주기**다.
- **PDM 채널 스케일**: API 레퍼런스 PDF에만 있는 값을 디코딩·교차검증하여 반영했다.
  버스/온도 스케일은 `epsbat.h` 와 일치 확인됨. 정밀도가 중요하면 원문 재확인 권장.
- 오류 값은 프레임에 `ERR` 로 표기된다. 원인은 `EPS_GetLastError()` 로 확인 가능.
