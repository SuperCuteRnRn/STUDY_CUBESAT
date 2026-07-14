# ============================================================================
# Makefile — ITSDL_TXU_TEST_CODE (TRXVU flat API 배포판)
# ============================================================================
# Target: Linux (Raspberry Pi / ARM / x86) with I2C (linux/i2c-dev.h)
#
#   make            통합 테스트 프로그램 txu_test 빌드
#   make lib        정적 라이브러리 libtxu.a
#   make shared     공유 라이브러리 libtxu.so
#   make dist       배포 패키지 dist/ (헤더 + 라이브러리 + 문서)
#   make clean      산출물 제거
#
# 실행 (OBC): sudo ./txu_test
# ============================================================================

CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -std=c99 -O2 -D_DEFAULT_SOURCE
LDFLAGS ?=

# 라이브러리 소스 = flat wrapper + 저수준 3종 (테스트 프로그램 제외)
LIB_SRCS = txu.c trxvu_i2c.c trxvu_commands.c trxvu_dev001.c
LIB_OBJS = $(LIB_SRCS:.c=.o)
PIC_OBJS = $(LIB_SRCS:.c=.pic.o)

# 통합 테스트 프로그램 (기본 빌드 대상)
TARGET   = txu_test

.PHONY: all lib shared dist clean rebuild

all: $(TARGET)

$(TARGET): txu_test.c $(LIB_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# ── 외부 배포용 라이브러리 ──────────────────────────────────────────
lib: libtxu.a
libtxu.a: $(LIB_OBJS)
	ar rcs $@ $^

shared: libtxu.so
%.pic.o: %.c
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<
libtxu.so: $(PIC_OBJS)
	$(CC) -shared -o $@ $^

# 배포 패키지: 라이브러리 2종 + 공개 헤더 txu.h + API 문서
dist: lib shared
	mkdir -p dist
	cp libtxu.a libtxu.so txu.h DOC_TXU_API_20260714.md dist/

clean:
	rm -f $(TARGET) $(LIB_OBJS) $(PIC_OBJS) libtxu.a libtxu.so
	rm -rf dist

rebuild: clean all
