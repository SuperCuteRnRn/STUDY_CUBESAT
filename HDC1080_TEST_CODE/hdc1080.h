/******************************************************************************
 * hdc1080.h — HDC1080 온·습도 센서 API (공개 헤더)
 *
 * TI HDC1080 온도·습도 센서를 Linux 상위 시스템에서 I2C로 제어하는 라이브러리.
 * 링크: gcc myapp.c libhdc1080.a -lpthread   (또는 libhdc1080.so를 ctypes로 호출)
 *
 * - I2C 버스 1(/dev/i2c-1), 슬레이브 주소 0x40 사용
 * - hdc1080_init()이 I2C 라이브러리 초기화와 센서 설정을 함께 수행한다
 * - 반환값 0 = 성공, 음수 = I2C 오류코드(i2c_lib.h의 I2C_ERR_*)
 ******************************************************************************/
#ifndef HDC1080_H
#define HDC1080_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── 온도 분해능 ─────────────────────────────────────────────────────── */
typedef enum
{
    Temperature_Resolution_14_bit = 0,
    Temperature_Resolution_11_bit = 1
} Temp_Reso;

/* ── 습도 분해능 ─────────────────────────────────────────────────────── */
typedef enum
{
    Humidity_Resolution_14_bit = 0,
    Humidity_Resolution_11_bit = 1,
    Humidity_Resolution_8_bit  = 2
} Humi_Reso;

/* 센서 초기화 — I2C 라이브러리 초기화 후 Configuration 레지스터에
 * 분해능과 측정 모드(온도+습도 순차 측정)를 설정한다.
 *   반환: 0 성공, 음수 I2C 오류코드(I2C_ERR_*) */
int hdc1080_init(Temp_Reso Temperature_Resolution_x_bit,
                 Humi_Reso Humidity_Resolution_x_bit);

/* 온도·습도 1회 측정.
 *   temperature [out] 섭씨 온도(°C)
 *   humidity    [out] 상대 습도(%RH)
 *   반환: 0 성공, 음수 I2C 오류코드(I2C_ERR_*) */
int hdc1080_start_measurement(float *temperature, float *humidity);

#ifdef __cplusplus
}
#endif

#endif /* HDC1080_H */
