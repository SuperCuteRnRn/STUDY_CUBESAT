# PLAN: eps_tlm_beacon 에 HDC1080 온·습도 통합 (RF 송출)

- 작성일: 2026-07-15
- 작업명: HDC_BEACON_MERGE

## 1. 작업 목적 및 배경

기존 예제 `Examples/eps_tlm_beacon` 은 EPSBAT 전원 텔레메트리를 AX.25 프레임으로
1초 주기 RF 송출한다(EPS_* + TXU_*). 여기에 **HDC1080 온·습도 센서 데이터
(`HT`=온도 °C, `HH`=습도 %RH)를 같은 텔레메트리 프레임에 추가**하여 함께 RF로
송출한다.

### 핵심 리스크 — I2C 라이브러리 심볼 충돌 (검증 완료)

`libepsbat.a` 와 `libhdc1080.a` 는 **둘 다 `I2C_*` 함수를 서로 다른 구현으로
정의**한다(에러카운트 타입 `uint8` vs `uint32_t`, `I2C_Open` 채널검사 방식 등 상이).
그대로 함께 링크하면 링커가 한쪽 `i2c_lib.o` 만 채택하는 **조용한 ODR 위반**이
발생한다(MISRA-C:2012 Rule 5.6/8.x — 식별자 고유성/정의 일관성 위반).

- 채널→버스 매핑은 양쪽 동일(channel 1 → `/dev/i2c-1`), 반환값 검사도 호환이라
  "우연히" 동작할 수는 있으나 **규격 위반이므로 격리한다.**

**해결책(검증 완료):** HDC 오브젝트(`hdc1080.o` + `i2c_lib.o`)를 `ld -r` 로 부분
링크한 뒤 `objcopy` 로 `hdc1080_init` / `hdc1080_start_measurement` **두 심볼만
전역 유지**하고 나머지 `I2C_*` 심볼을 **로컬화**한다. 그러면 HDC 는 자신의
`i2c_lib` 를, EPS 는 자신의 `i2c_lib` 를 각각 사용하며 충돌이 사라진다.
scratchpad 에서 EPS+TXU+HDC(격리) 동시 링크 성공(rc=0)을 확인했다.

## 2. 작업 범위 (변경 대상 파일 목록)

| 구분 | 경로 | 내용 |
|------|------|------|
| 수정 | `Examples/eps_tlm_beacon/eps_beacon.c` | HDC 초기화 + 텔레메트리 필드(HT/HH) 추가 |
| 수정 | `Examples/eps_tlm_beacon/Makefile` | HDC 격리 아카이브 빌드 + 링크(-lpthread) |
| 수정 | `STUDY_CUBESAT/Makefile` | `hdc` 타깃 추가, beacon 에 HDC_DIR 전달 |

- HDC1080 / EPSBAT / TXU 패키지의 소스·헤더·Makefile 은 **수정하지 않는다.**
- HDC 온도 변환/센싱은 벤더 API(`hdc1080_start_measurement`)에 위임(자체 변환식 없음).

## 3. 구현 방법 (단계별 상세 절차)

### 3.1 `eps_beacon.c` 수정

#### (a) 헤더 포함 추가 — 변경 전/후

변경 전(25~26행):
```c
#include "epsbat.h"
#include "txu.h"
```
변경 후:
```c
#include "epsbat.h"
#include "txu.h"
#include "hdc1080.h"
```

#### (b) HDC 초기화 상태 플래그(파일 스코프 static) 추가

`GS_CALLSIGN`/`SAT_CALLSIGN` 정의 아래(51행 부근)에 추가:
```c
/* ── HDC1080 온·습도 초기화 상태 (0 = 미초기화/실패) ───────────────────── */
static uint8_t g_hdc_ok = 0U;
```

#### (c) `build_telemetry()` 에 HDC 필드 추가 — 리셋 카운터 append 직전에 삽입

`build_telemetry()` 의 리셋 카운터 4종(`BOR/AR/MR/CWR`) append **바로 앞**에 삽입:
```c
    /* HDC1080 온·습도 (HT=온도 °C, HH=습도 %RH) */
    {
        float   tc  = 0.0f;
        float   rh  = 0.0f;
        uint8_t hok = 0U;
        if (g_hdc_ok != 0U)
        {
            if (hdc1080_start_measurement(&tc, &rh) == 0)
            {
                hok = 1U;
            }
        }
        used = buf_append(buf, size, used, "%s=%.2f ", "HT", (double)tc, hok);
        used = buf_append(buf, size, used, "%s=%.2f ", "HH", (double)rh, hok);
    }

    /* 리셋 카운터 4종 */
    used = buf_append_u(buf, size, used, "BOR", EPS_GetNumberOfBrownOutReset());
```
(주: 마지막 `BOR` 줄은 기존 줄이며, 삽입 위치 식별용으로만 표기)

#### (d) `main()` 에서 HDC 초기화 — EPS 버전 확인 통과 후(else 블록 진입 직후)

변경 전(295~297행):
```c
    else
    {
        uint32_t seq = 0U;
```
변경 후:
```c
    else
    {
        uint32_t seq = 0U;

        /* HDC1080 온·습도 센서 초기화 (14bit 온도/습도) */
        g_hdc_ok = (hdc1080_init(Temperature_Resolution_14_bit,
                                 Humidity_Resolution_14_bit) == 0) ? 1U : 0U;
        if (g_hdc_ok == 0U)
        {
            (void)printf("(!) HDC1080 init 실패 — HT/HH 는 ERR 로 송출됩니다.\n");
        }
```

#### (e) 파일 헤더 주석에 HDC 사용/의존 명시(1~14행 블록 내 1줄 추가)

`epsbat.h(EPS_*) + txu.h(TXU_*)` 설명 줄에 HDC 를 명시하고, pthread 링크 필요를
주석에 남긴다(문서화 목적, 코드 동작 무관).

### 3.2 `Examples/eps_tlm_beacon/Makefile` 수정

HDC 헤더 경로/격리 아카이브 규칙 추가. 변경 후 전체 관련 부분:
```makefile
# 헤더/라이브러리 경로 (본 예제 폴더 기준 상대경로 기본값)
EPS_DIR ?= ../../ITSDL_EPSBAT_TEST_CODE/dist
TXU_DIR ?= ../../ITSDL_TXU_TEST_CODE/dist
HDC_DIR ?= ../../HDC1080_TEST_CODE/Release_HDC1080_20260715/library

INCLUDES    = -I$(EPS_DIR) -I$(TXU_DIR) -I$(HDC_DIR)
STATIC_LIBS = $(EPS_DIR)/libepsbat.a $(TXU_DIR)/libtxu.a libhdc1080_iso.a
LDLIBS      = -lpthread

# HDC i2c_lib 심볼 격리 아카이브 — EPS i2c_lib 와의 ODR 충돌 방지
# hdc1080_init / hdc1080_start_measurement 만 전역 유지, I2C_* 는 로컬화
libhdc1080_iso.a: $(HDC_DIR)/libhdc1080.a
	rm -rf hdc_iso && mkdir hdc_iso
	cd hdc_iso && ar x $(abspath $(HDC_DIR)/libhdc1080.a)
	ld -r hdc_iso/hdc1080.o hdc_iso/i2c_lib.o -o hdc_iso/hdc1080_iso.o
	objcopy --keep-global-symbol=hdc1080_init \
	        --keep-global-symbol=hdc1080_start_measurement \
	        hdc_iso/hdc1080_iso.o
	ar rcs $@ hdc_iso/hdc1080_iso.o

# 방법 1 (권장): 정적 링크
eps_beacon: eps_beacon.c libhdc1080_iso.a
	$(CC) $(CFLAGS) $(INCLUDES) eps_beacon.c $(STATIC_LIBS) $(LDLIBS) -o eps_beacon
```
- 기존 `EPS_DIR`/`TXU_DIR` 기본값도 실제 경로(`.../dist`)로 정정한다(현재 예제
  단독 `make` 가 헤더를 못 찾는 문제도 함께 해결).
- `shared` / `clean` 타깃은 `libhdc1080_iso.a`, `hdc_iso/` 정리 추가.

### 3.3 최상위 `STUDY_CUBESAT/Makefile` 수정

```makefile
EPS_DIR := ITSDL_EPSBAT_TEST_CODE
TXU_DIR := ITSDL_TXU_TEST_CODE
HDC_DIR := HDC1080_TEST_CODE
EX_DIR  := Examples/eps_tlm_beacon

.PHONY: all epsbat txu hdc beacon clean

all: beacon

epsbat:
	$(MAKE) -C $(EPS_DIR) all dist

txu:
	$(MAKE) -C $(TXU_DIR) all dist

hdc:
	$(MAKE) -C $(HDC_DIR) all dist

beacon: epsbat txu hdc
	$(MAKE) -C $(EX_DIR) eps_beacon \
	    EPS_DIR=$(CURDIR)/$(EPS_DIR)/dist \
	    TXU_DIR=$(CURDIR)/$(TXU_DIR)/dist \
	    HDC_DIR=$(CURDIR)/$(HDC_DIR)/Release_HDC1080_20260715/library

clean:
	$(MAKE) -C $(EPS_DIR) clean
	$(MAKE) -C $(TXU_DIR) clean
	$(MAKE) -C $(HDC_DIR) clean
	$(MAKE) -C $(EX_DIR)  clean
```

## 4. 예상 영향 범위 및 리스크

- **송출 프레임 크기 증가**: `HT=..`, `HH=..` 두 필드(각 ~10 byte) 추가 →
  `TLM_BUF_SIZE(512)` / `FRAME_MAX(200)` 여유 내(현재 사용량 « 512). 문제 없음.
- **ODR 충돌**: 3.1~3.2 의 심볼 격리로 제거(검증 완료).
- **런타임 의존성**: HDC 격리 `i2c_lib` 가 pthread 사용 → `-lpthread` 링크 필수(반영).
- **하드웨어 부재 시**: 이 개발 PC 에는 실제 I2C 디바이스가 없어 런타임 측정은
  실패할 수 있음 → `g_hdc_ok`/`hok` 플래그로 **HT/HH=ERR** 로 안전 표기, EPS
  텔레메트리 송출은 계속됨. 실장(OBC) 검증은 별도.
- **HDC 아카이브 멤버명 의존**: 격리 규칙이 `hdc1080.o`/`i2c_lib.o` 멤버명을 가정.
  현재 아카이브에서 확인됨. 이름 변경 시 규칙 수정 필요(주석 명시).

### MISRA-C:2012 준수/이탈

- 추가 코드는 단일출구(Rule 15.5), 초기화(모든 변수 선언 시 초기화), switch 미사용,
  매직넘버 회피(라벨은 리터럴 문자열 키), 명시적 캐스트((double)tc) 적용.
- 기존 파일의 `snprintf`(Rule 21.6) 이탈 주석 방침을 그대로 따른다(신규 추가 없음).
- I2C 심볼 격리는 **ODR 위반을 제거하는 조치**로, 신규 이탈을 만들지 않는다.

## 5. 완료 기준 (검증 방법)

`STUDY_CUBESAT/` 에서:
```bash
make            # 전체 빌드 성공(경고 허용)
```
확인:
```bash
test -x Examples/eps_tlm_beacon/eps_beacon
nm --defined-only Examples/eps_tlm_beacon/eps_beacon | grep -c ' T I2C_'   # 단일 I2C 세트만
```
dry-run 동작 확인(하드웨어 없이 프레임에 HT/HH 포함 여부):
```bash
# TX_ENABLE=0 으로 재빌드하여 프레임 출력에 HT=/HH= 필드가 포함되는지 확인
make -C Examples/eps_tlm_beacon clean
make -C Examples/eps_tlm_beacon eps_beacon CFLAGS='-std=c99 -Wall -Wextra -O2 -DTX_ENABLE=0' \
     EPS_DIR=... TXU_DIR=... HDC_DIR=...
./Examples/eps_tlm_beacon/eps_beacon | head   # "HT=... HH=..." 필드 존재 확인
```
(주: `TX_ENABLE` 는 소스 `#define` 이므로 dry-run 확인은 소스 임시수정 또는 별도
빌드 매크로가 필요. 실제 RF 송출은 규제·허가 확인된 환경에서만.)
