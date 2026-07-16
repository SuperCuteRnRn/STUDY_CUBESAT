# ============================================================================
# Makefile — cmd_rx_img_tx (프리셋10 수신 시 촬영·이미지 조각 전송)
# ----------------------------------------------------------------------------
#   make          rx_img_tx 빌드
#   make clean    산출물 제거
#
# I2C 계층: 검증된 벤더 라이브러리(i2c_lib.c + trx_rcv_function.c)를 함께 컴파일.
# 실행 (OBC, I2C 접근): sudo ./rx_img_tx
# 카메라: rpicam-still 이 PATH 에 있어야 함(라즈베리파이 OS).
# ============================================================================

CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -std=c99 -O2 -D_DEFAULT_SOURCE
LDLIBS   = -lpthread
SRCS     = rx_img_tx.c i2c_lib.c trx_rcv_function.c
TARGET   = rx_img_tx

.PHONY: all clean rebuild

all: $(TARGET)

$(TARGET): $(SRCS) i2c_lib.h trx_rcv_function.h
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LDLIBS)

clean:
	rm -f $(TARGET)

rebuild: clean all
