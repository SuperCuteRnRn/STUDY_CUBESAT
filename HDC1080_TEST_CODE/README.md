# HDC1080 온·습도 센서 테스트 코드

TI **HDC1080** 온도·습도 센서를 Linux 상위 시스템(라즈베리파이 등)에서
I2C로 제어하는 드라이버 및 테스트 프로그램이다.

- 슬레이브 주소: **0x40**, I2C 버스 **1**(`/dev/i2c-1`)

## 파일 구성

| 파일 | 설명 |
|---|---|
| `hdc1080.c` / `hdc1080.h` | HDC1080 드라이버 (공개 API) |
| `i2c_lib.c` / `i2c_lib.h` | Linux I2C 디바이스 래퍼 |
| `main.c` | 테스트 프로그램(온도/습도 반복 출력) |
| `DOC_HDC1080_API_20260715.md` | 라이브러리 API 레퍼런스 |

## 빌드 및 실행

```bash
make            # 테스트 프로그램(main) 빌드
sudo ./main     # 온도/습도 반복 출력

make doc-pdf    # API 문서 PDF 생성(DOC_HDC1080_API_20260715.pdf)
make dist       # 배포 패키지 생성(Release_HDC1080_20260715/)
```

## 배포용 라이브러리

`make dist` 실행 시 `Release_HDC1080_20260715/`에 아래가 생성된다.

- `library/libhdc1080.a`, `libhdc1080.so`, `hdc1080.h`
- `docs/DOC_HDC1080_API_20260715.md`, `.pdf`

사용법은 API 레퍼런스 2~3절 참조:

```bash
gcc myapp.c libhdc1080.a -lpthread -o myapp   # C 정적 링크
```
