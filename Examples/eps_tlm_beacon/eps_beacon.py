#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""EPSBAT 전체 텔레메트리를 1초 주기로 AX.25 프레임(To=ITSDGS-0, From=ITSSAT-0)에
실어 TXU 로 송출하는 예제. 공개 API(libepsbat.so + libtxu.so)만 사용.

(!) TXU_TrsSendFrameOverride 는 실제 RF 를 송출한다(HIGH RISK).
    운용 규제/주파수 허가/안테나 정합이 확인된 환경에서만 실행하라.
    검증 단계에서는 TX_ENABLE = False(dry-run) 권장.
"""
import ctypes as C
import signal
import sys
import time

TX_ENABLE   = True          # False = dry-run(송출 없이 출력)
PERIOD_SEC  = 1.0
FRAME_MAX   = 200
EPS_ERR_U16 = 0xFFFF

# ── 라이브러리 로드(실행 파일 옆에 .so 배치) ──────────────────────────────
eps = C.CDLL("./libepsbat.so")
txu = C.CDLL("./libtxu.so")

# EPS 시그니처
eps.EPS_GetVersion.restype     = C.c_uint16
eps.EPS_GetBoardStatus.restype = C.c_uint16
eps.EPS_GetTelemetry.restype   = C.c_uint16
eps.EPS_GetTelemetry.argtypes  = [C.c_uint16]
for _fn in ("EPS_GetNumberOfBrownOutReset", "EPS_GetNumberOfAutomaticReset",
            "EPS_GetNumberOfManuelReset", "EPS_GetNumberOfCommWatchdogReset"):
    getattr(eps, _fn).restype = C.c_uint16

# TXU 시그니처: int TXU_TrsSendFrameOverride(u8 to[7],u8 from[7],u8* frame,u8 len,u8* rem)
CS7 = C.c_ubyte * 7
txu.TXU_TrsSendFrameOverride.restype  = C.c_int
txu.TXU_TrsSendFrameOverride.argtypes = [CS7, CS7, C.POINTER(C.c_ubyte),
                                         C.c_uint8, C.POINTER(C.c_ubyte)]
txu.TXU_Close.restype = None

# ── 콜사인(6 ASCII + 1 SSID) ──────────────────────────────────────────────
TO_CS   = CS7(*b"ITSDGS", 0)   # 지상국 ITSDGS-0
FROM_CS = CS7(*b"ITSSAT", 0)   # 위성   ITSSAT-0

# ── 텔레메트리 정의 ───────────────────────────────────────────────────────
BUS = [  # (code, label, scale)
    (0xE220, "VB",  0.008978), (0xE224, "IB",  0.00681989),
    (0xE230, "V12", 0.01349),  (0xE234, "I12", 0.00206663),
    (0xE210, "V5",  0.005865), (0xE214, "I5",  0.00681989),
    (0xE200, "V3",  0.004311), (0xE204, "I3",  0.00681989),
]
TEMP = [(0xE308, "T1"), (0xE388, "T2")]  # 'C = 0.372434*count - 273.15
VSW1 = 0xE410
PDM  = [  # (ch, vscale, iscale) — API 레퍼런스 Rev1.2 4절
    (1, 0.01349, 0.0013283), (2, 0.01349, 0.0013283),
    (3, 0.008993, 0.0062395), (4, 0.008993, 0.0062395),
    (5, 0.005865, 0.0013288), (6, 0.005865, 0.0013288),
    (8, 0.004311, 0.0013284), (9, 0.004311, 0.0013284),
]

_run = True


def _stop(_sig, _frm):
    global _run
    _run = False


signal.signal(signal.SIGINT, _stop)


def read_scaled(code, scale, fmt):
    raw = eps.EPS_GetTelemetry(code)
    return "ERR" if raw == EPS_ERR_U16 else fmt % (raw * scale)


def read_temp(code):
    raw = eps.EPS_GetTelemetry(code)
    return "ERR" if raw == EPS_ERR_U16 else "%.1f" % (0.372434 * raw - 273.15)


def build_telemetry(seq):
    p = ["ITSSAT TLM SEQ=%d ST=0x%04X" % (seq, eps.EPS_GetBoardStatus())]
    for code, label, scale in BUS:
        p.append("%s=%s" % (label, read_scaled(code, scale, "%.2f")))
    for code, label in TEMP:
        p.append("%s=%s" % (label, read_temp(code)))
    for ch, vs, is_ in PDM:
        vcode = VSW1 + 16 * (ch - 1)
        p.append("P%dV=%s" % (ch, read_scaled(vcode, vs, "%.2f")))
        p.append("P%dI=%s" % (ch, read_scaled(vcode + 4, is_, "%.3f")))
    p.append("BOR=%d" % eps.EPS_GetNumberOfBrownOutReset())
    p.append("AR=%d" % eps.EPS_GetNumberOfAutomaticReset())
    p.append("MR=%d" % eps.EPS_GetNumberOfManuelReset())
    p.append("CWR=%d" % eps.EPS_GetNumberOfCommWatchdogReset())
    return " ".join(p).encode("ascii")


def send_frame(payload):
    for off in range(0, len(payload), FRAME_MAX):
        chunk = payload[off:off + FRAME_MAX]
        buf = (C.c_ubyte * len(chunk)).from_buffer_copy(chunk)
        if TX_ENABLE:
            rem = C.c_ubyte(0)
            rc = txu.TXU_TrsSendFrameOverride(TO_CS, FROM_CS, buf,
                                              len(chunk), C.byref(rem))
            if rc != 0:
                return -1
        else:
            print("[DRY-RUN TX %d B] %s" % (len(chunk), chunk.decode("ascii")))
    return 0


def main():
    print("=== EPSBAT TLM beacon  To=ITSDGS-0  From=ITSSAT-0 ===")
    print("(!) RF 송출" if TX_ENABLE else "(dry-run)", "— Ctrl-C 로 중단")
    if eps.EPS_GetVersion() != 0x0001:
        print("EPS board not found (version mismatch).")
        return 1
    seq = 0
    while _run:
        t0 = time.monotonic()
        if send_frame(build_telemetry(seq)) != 0:
            print("TX error (seq=%d)" % seq)
        seq += 1
        dt = PERIOD_SEC - (time.monotonic() - t0)
        if dt > 0:
            time.sleep(dt)
    print("\nstopped by user.")
    txu.TXU_Close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
