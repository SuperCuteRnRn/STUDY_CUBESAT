# ============================================================================
# Makefile for Clyde Space EPS Test Code
# ============================================================================
# Target: Linux (Embedded ARM or x86) with I2C support
# ============================================================================

CC 	= gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS =

# Common source files (library)
COMMON_SRCS = function.c i2c_lib.c
COMMON_OBJS = $(COMMON_SRCS:.c=.o)

# 통합 테스트 프로그램 (기본 빌드 대상) — 모든 기능을 메뉴 하나로 테스트
TARGETS = eps_test

# 구형 개별 프로그램 (필요 시 `make legacy`)
LEGACY_TARGETS = main for_main repeat eps_numberofreset \
          all_off_initial_pdm n_on_initial_pdm n_off_initial_pdm \
          n_on_pdm n_off_pdm EPS_RW_TEST pcm_monitor

# Default target
all: $(TARGETS)

legacy: $(LEGACY_TARGETS)

eps_test: eps_test.c $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Individual executables
main: main.c $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

for_main: for_main.c $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

repeat: repeat.c $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

eps_numberofreset: eps_numberofreset.c $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

all_off_initial_pdm: all_off_initial_pdm.c $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

n_on_initial_pdm: n_on_initial_pdm.c $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

n_off_initial_pdm: n_off_initial_pdm.c $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

n_on_pdm: n_on_pdm.c $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

n_off_pdm: n_off_pdm.c $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

EPS_RW_TEST: EPS_RW_TEST.c $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

pcm_monitor: pcm_monitor.c $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Object file compilation
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# ── 외부 배포용 라이브러리 ──────────────────────────────────────────
LIB_SRCS = function.c i2c_lib.c
LIB_OBJS = $(LIB_SRCS:.c=.o)
PIC_OBJS = $(LIB_SRCS:.c=.pic.o)

lib: libepsbat.a
libepsbat.a: $(LIB_OBJS)
	ar rcs $@ $^

shared: libepsbat.so
%.pic.o: %.c
	$(CC) $(CFLAGS) -fPIC -c $< -o $@
libepsbat.so: $(PIC_OBJS)
	$(CC) -shared -o $@ $^
	strip --strip-unneeded $@

# 배포 패키지: dist/ = 라이브러리 2종 + 공개 헤더 + API 문서
dist: lib shared
	mkdir -p dist
	cp libepsbat.a libepsbat.so epsbat.h DOC_EPSBAT_API_20260712.md dist/

# Clean up
clean:
	rm -f $(TARGETS) $(LEGACY_TARGETS) $(COMMON_OBJS) \
	      $(PIC_OBJS) libepsbat.a libepsbat.so
	rm -rf dist

# Alias for clean and rebuild
rebuild: clean all

.PHONY: all clean rebuild
