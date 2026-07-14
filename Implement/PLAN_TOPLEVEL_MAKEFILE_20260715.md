# PLAN: 최상위 통합 Makefile 추가 (STUDY_CUBESAT에서 `make` 전체 빌드)

- 작성일: 2026-07-15
- 작업명: TOPLEVEL_MAKEFILE

## 1. 작업 목적 및 배경

현재 `STUDY_CUBESAT/` 최상위에는 빌드 진입점이 없어, 각 하위 프로젝트
(`ITSDL_EPSBAT_TEST_CODE`, `ITSDL_TXU_TEST_CODE`, `Examples/eps_tlm_beacon`)를
개별적으로 `make` 해야 한다. 특히 예제 `eps_beacon`은 두 배포 라이브러리
(`libepsbat.a`, `libtxu.a`)에 정적 링크되는데, 라이브러리가 먼저 `make dist`로
생성되어 있지 않으면 빌드가 실패한다(현재 `epsbat.h: No such file` 및 라이브러리
부재로 실패 중).

목표: **`STUDY_CUBESAT/` 위치에서 `make` 한 번으로 전부 컴파일**되도록 최상위
Makefile을 추가한다.

## 2. 작업 범위 (변경 대상 파일 목록)

| 구분 | 경로 | 내용 |
|------|------|------|
| 신규 | `STUDY_CUBESAT/Makefile` | 최상위 통합 Makefile (하위 Makefile 위임) |

- **하위 프로젝트의 기존 Makefile/소스는 수정하지 않는다.** (위임 호출만 함)
- 산출물: 각 하위 폴더의 실행파일/`.o`/`libepsbat.*`/`libtxu.*`/`dist/`,
  그리고 `Examples/eps_tlm_beacon/eps_beacon`.

## 3. 구현 방법 (단계별 상세 절차)

빌드 순서(의존관계):
1. `ITSDL_EPSBAT_TEST_CODE` → `make all dist` (eps_test + libepsbat.a/.so + dist/)
2. `ITSDL_TXU_TEST_CODE` → `make all dist` (txu_test + libtxu.a/.so + dist/)
3. `Examples/eps_tlm_beacon` → `make eps_beacon`, 단 `EPS_DIR`/`TXU_DIR`를
   위에서 생성된 각 `dist/`의 **절대경로**로 덮어쓴다(상대경로 혼동 방지).

최상위 Makefile은 위 세 단계를 재귀 `$(MAKE) -C`로 위임한다.

### 추가되는 코드 전체 — `STUDY_CUBESAT/Makefile` (신규)

```makefile
# ============================================================================
# STUDY_CUBESAT 통합 빌드 Makefile
# ----------------------------------------------------------------------------
#   make            EPSBAT + TXU 라이브러리/테스트 + eps_tlm_beacon 예제 전체 빌드
#   make epsbat     EPSBAT 테스트 + 배포 라이브러리(dist/)만
#   make txu        TXU 테스트 + 배포 라이브러리(dist/)만
#   make beacon     예제 eps_beacon (epsbat, txu 선행)
#   make clean      전체 산출물 제거
# ----------------------------------------------------------------------------
# 하위 프로젝트의 기존 Makefile을 그대로 위임 호출한다(하위 파일 무수정).
# ============================================================================

EPS_DIR := ITSDL_EPSBAT_TEST_CODE
TXU_DIR := ITSDL_TXU_TEST_CODE
EX_DIR  := Examples/eps_tlm_beacon

.PHONY: all epsbat txu beacon clean

# 기본 타깃: 예제까지 전부 (beacon 이 epsbat/txu 를 선행 의존)
all: beacon

# EPSBAT: 통합 테스트(eps_test) + 배포 라이브러리(libepsbat.a/.so, dist/)
epsbat:
	$(MAKE) -C $(EPS_DIR) all dist

# TXU: 통합 테스트(txu_test) + 배포 라이브러리(libtxu.a/.so, dist/)
txu:
	$(MAKE) -C $(TXU_DIR) all dist

# 예제: 두 라이브러리 dist/ 를 절대경로로 지정해 정적 링크
beacon: epsbat txu
	$(MAKE) -C $(EX_DIR) eps_beacon \
	    EPS_DIR=$(CURDIR)/$(EPS_DIR)/dist \
	    TXU_DIR=$(CURDIR)/$(TXU_DIR)/dist

clean:
	$(MAKE) -C $(EPS_DIR) clean
	$(MAKE) -C $(TXU_DIR) clean
	$(MAKE) -C $(EX_DIR)  clean
```

## 4. 예상 영향 범위 및 리스크

- 하위 프로젝트 파일 무변경 → 기존 개별 빌드 흐름에 영향 없음.
- `beacon` 이 `epsbat`, `txu` 를 선행 의존으로 두므로 라이브러리 미빌드로 인한
  링크 실패가 구조적으로 제거된다.
- 병렬 빌드(`make -j`) 시 재귀 타깃 간 순서 보장을 위해 의존관계를 명시했다.
- 사전 문법검사(`-fsyntax-only`) 결과: EPSBAT/TXU/예제 소스 전부 컴파일 성공
  (경고만 존재: `i2c_lib.c`의 `usleep` implicit declaration, unused parameter 등).
  이 경고들은 하위 소스 이슈로 본 계획 범위 밖이며 빌드는 정상 통과한다.

### MISRA 관련

- 본 산출물은 Makefile(빌드 스크립트)로 C 코드가 아니므로 MISRA-C:2012 적용 대상
  아님. 하위 C 소스는 수정하지 않는다.

## 5. 완료 기준 (검증 방법)

`STUDY_CUBESAT/` 에서 아래 실행 시 성공:

```bash
make            # 전체 빌드, 에러 없이 종료(경고 허용)
```

빌드 후 다음 산출물 존재 확인:

```bash
test -f ITSDL_EPSBAT_TEST_CODE/dist/libepsbat.a
test -f ITSDL_TXU_TEST_CODE/dist/libtxu.a
test -x ITSDL_EPSBAT_TEST_CODE/eps_test
test -x ITSDL_TXU_TEST_CODE/txu_test
test -x Examples/eps_tlm_beacon/eps_beacon
```

`make clean` 후 위 산출물이 모두 제거되면 정상.
