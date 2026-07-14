# EPSBAT TLM beacon 예제 — libepsbat + libtxu 동시 링크 (Linux/ARM 타겟 전용)
#
# 라이브러리(.a/.so)는 각 배포 패키지에서 `make dist` 후 생성된다.
# 경로가 다르면 EPS_DIR / TXU_DIR 을 make 인자로 덮어쓴다:
#   make EPS_DIR=/path/to/epsbat/lib TXU_DIR=/path/to/txu

CC      ?= gcc
CFLAGS  ?= -std=c99 -Wall -Wextra -O2

# 헤더/라이브러리 경로 (본 예제 폴더 기준 상대경로 기본값)
EPS_DIR ?= ../../Release_ITSDL_EPSBAT_20260712/library
TXU_DIR ?= ../../ITSDL_TXU_TEST_CODE

INCLUDES    = -I$(EPS_DIR) -I$(TXU_DIR)
STATIC_LIBS = $(EPS_DIR)/libepsbat.a $(TXU_DIR)/libtxu.a

# 방법 1 (권장): 정적 링크 — 실행 파일에 라이브러리 포함, 어디서든 실행
eps_beacon: eps_beacon.c
	$(CC) $(CFLAGS) $(INCLUDES) eps_beacon.c $(STATIC_LIBS) -o eps_beacon

# 방법 2: 공유 링크 — libepsbat.so / libtxu.so 를 실행 파일 옆에서 로드
shared: eps_beacon.c
	$(CC) $(CFLAGS) $(INCLUDES) eps_beacon.c \
	    -L$(EPS_DIR) -L$(TXU_DIR) -lepsbat -ltxu \
	    -Wl,-rpath,'$$ORIGIN' -o eps_beacon

clean:
	rm -f eps_beacon

.PHONY: shared clean
