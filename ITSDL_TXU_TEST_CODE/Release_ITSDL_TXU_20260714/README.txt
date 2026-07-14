ITSDL_TRXVU 배포 패키지 (2026-07-14)
=======================================

docs/
  ITSDL_TXU_API_Reference_Rev1.0.pdf      라이브러리 flat API 레퍼런스 (C/Python 예제) — 배포용
  ITSDL_TXU_README.pdf                    사용법/빌드/명령 요약 — 배포용
  ITSDL_TXU_ICD_Reference.pdf             원 프로토콜(ISIS TRXVU ICD-0001/0002) 참조 안내
  (원본 마크다운: DOC_TXU_API_20260714.md, ICD_REFERENCE.txt — 재생성은 ../md2pdf.py)

library/
  txu.h                                   공개 헤더 (TXU_* 함수 선언 + 상수)
  libtxu.a / libtxu.so                    * 라즈베리파이/ARM 타겟에서 `make dist` 실행 후
                                            ITSDL_TXU_TEST_CODE/dist/ 에서 이 폴더로 복사

slave 주소: RCV 0x60 (Receiver) / TRS 0x61 (Transmitter)
I2C 디바이스: 기본 /dev/i2c-1 (TXU_SetDevice()로 변경)

사용법 요약: DOC_TXU_API 2~3절 참조
  C:      gcc myapp.c -L. -ltxu -o myapp
  Python: ctypes.CDLL("./libtxu.so")

(!) 위험 명령: RF 송출(SendFrame/Beacon), 주파수 변경(SetFrequency),
    콜사인 영구변경(SetCallsign), 보드 리셋(Reset) — 호출 전 운용 규제·안전 절차 확인.
    상세 위험 등급표는 DOC_TXU_API 6절 참조.
