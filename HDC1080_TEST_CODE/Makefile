# ============================================================================
# Makefile for HDC1080 온·습도 센서 라이브러리 / 테스트
# ============================================================================
# Target: Linux (Raspberry Pi 등) with I2C support
# ============================================================================

CC      = gcc
CFLAGS  = -Wall -Wextra -O2
LDFLAGS =
LDLIBS  = -lpthread

# 공통 라이브러리 소스
LIB_SRCS = hdc1080.c i2c_lib.c
LIB_OBJS = $(LIB_SRCS:.c=.o)
PIC_OBJS = $(LIB_SRCS:.c=.pic.o)

# 테스트 프로그램(기본 빌드 대상)
TARGET = main

# 배포 패키지 디렉토리 / 문서
RELEASE_DIR = Release_HDC1080_20260715
API_DOC     = DOC_HDC1080_API_20260715.md
API_PDF     = DOC_HDC1080_API_20260715.pdf

all: $(TARGET)

$(TARGET): main.c $(LIB_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# ── 외부 배포용 라이브러리 ──────────────────────────────────────────
lib: libhdc1080.a
libhdc1080.a: $(LIB_OBJS)
	ar rcs $@ $^

shared: libhdc1080.so
%.pic.o: %.c
	$(CC) $(CFLAGS) -fPIC -c $< -o $@
libhdc1080.so: $(PIC_OBJS)
	$(CC) -shared -o $@ $^ -lpthread
	strip --strip-unneeded $@

# ── API 문서 PDF 생성(도구가 준비된 경우) ───────────────────────────
doc-pdf: $(API_PDF)
$(API_PDF): $(API_DOC) scripts/md2pdf.py
	python3 scripts/md2pdf.py $(API_DOC) $(API_PDF)

# ── 배포 패키지: library = 라이브러리 2종 + 공개 헤더, docs = MD+PDF ──
dist: lib shared
	mkdir -p $(RELEASE_DIR)/library $(RELEASE_DIR)/docs
	cp libhdc1080.a libhdc1080.so hdc1080.h $(RELEASE_DIR)/library/
	cp $(API_DOC) $(RELEASE_DIR)/docs/
	@if [ -f $(API_PDF) ]; then cp $(API_PDF) $(RELEASE_DIR)/docs/; \
	  else echo "note: $(API_PDF) 없음 — 'make doc-pdf' 먼저 실행"; fi
	cp README.md $(RELEASE_DIR)/

clean:
	rm -f $(TARGET) $(LIB_OBJS) $(PIC_OBJS) libhdc1080.a libhdc1080.so $(API_PDF)
	rm -rf $(RELEASE_DIR)

rebuild: clean all

.PHONY: all lib shared dist doc-pdf clean rebuild
