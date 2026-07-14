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
