ITSDL_EPS&BAT 배포 패키지 (2026-07-12)
=======================================

docs/
  ITSDL_EPSBAT_Datasheet_Rev1.1.pdf       보드 데이터시트 (사양·I2C 프로토콜)
  ITSDL_EPSBAT_API_Reference_Rev1.2.pdf   라이브러리 API 레퍼런스 (C/Python 예제)
  ITSDL_EPSBAT_Operation_Manual.pdf       운용 매뉴얼

library/
  epsbat.h                                공개 헤더 (함수 선언 + TLE 상수)
  libepsbat.a / libepsbat.so              * 라즈베리파이에서 `make dist` 실행 후
                                            EPS_TEST_CODE/dist/ 에서 이 폴더로 복사

사용법 요약: API 레퍼런스 2~3절 참조
  C:      gcc myapp.c -L. -lepsbat -o myapp
  Python: ctypes.CDLL("./libepsbat.so")
