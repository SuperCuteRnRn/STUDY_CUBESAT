#include "hdc1080.h"
#include "i2c_lib.h"
#include <stddef.h>

/* ── 디바이스/레지스터 상수 ─────────────────────────────────────────── */
#define HDC1080_I2C_CHANNEL     1U      /* /dev/i2c-1 */
#define HDC1080_DEV_ADDR        0x40U
#define HDC1080_REG_CONFIG      0x02U
#define HDC1080_REG_TEMPERATURE 0x00U
#define HDC1080_REG_HUMIDITY    0x01U

/* ── Configuration 레지스터 비트 ────────────────────────────────────── */
#define HDC1080_CFG_MODE_BOTH   0x1000U     /* bit12=1: 온도+습도 순차 측정 */
#define HDC1080_CFG_TRES_11BIT  (1U << 10)  /* bit10 : 온도 11비트           */
#define HDC1080_CFG_HRES_11BIT  (1U << 8)   /* bit8  : 습도 11비트           */
#define HDC1080_CFG_HRES_8BIT   (1U << 9)   /* bit9  : 습도 8비트            */

/* ── 변환 상수 ──────────────────────────────────────────────────────── */
#define HDC1080_MEAS_DELAY_MS   15U         /* 14비트 온도+습도 변환 시간(ms) */
#define HDC1080_ADC_FULL_SCALE  65536.0f
#define HDC1080_TEMP_SPAN       165.0f
#define HDC1080_TEMP_OFFSET     40.0f
#define HDC1080_HUMI_SPAN       100.0f
#define HDC1080_READ_LEN        4U

int hdc1080_init(Temp_Reso Temperature_Resolution_x_bit,
                 Humi_Reso Humidity_Resolution_x_bit)
{
    uint16_t config_reg_value = HDC1080_CFG_MODE_BOTH;
    uint8_t  data_send[2] = {0};
    int      ret = 0;

    ret = I2C_LibInit();
    if (ret == I2C_SUCCESS)
    {
        if (Temperature_Resolution_x_bit == Temperature_Resolution_11_bit)
        {
            config_reg_value |= HDC1080_CFG_TRES_11BIT;
        }

        switch (Humidity_Resolution_x_bit)
        {
        case Humidity_Resolution_11_bit:
            config_reg_value |= HDC1080_CFG_HRES_11BIT;
            break;
        case Humidity_Resolution_8_bit:
            config_reg_value |= HDC1080_CFG_HRES_8BIT;
            break;
        default:
            break;
        }

        data_send[0] = (uint8_t)(config_reg_value >> 8);
        data_send[1] = (uint8_t)(config_reg_value & 0x00FFU);

        ret = I2C_WriteBytes(HDC1080_I2C_CHANNEL, HDC1080_DEV_ADDR,
                             HDC1080_REG_CONFIG, 2U, data_send);
    }

    return ret;
}

int hdc1080_start_measurement(float *temperature, float *humidity)
{
    uint8_t  receive_data[HDC1080_READ_LEN] = {0};
    uint16_t temp_x = 0;
    uint16_t humi_x = 0;
    int      ret = 0;

    if ((temperature == NULL) || (humidity == NULL))
    {
        ret = I2C_ERR_WRITE_READ;   /* 잘못된 인자 */
    }
    else
    {
        /* 온도 레지스터부터 온도+습도 4바이트 순차 읽기 */
        ret = I2C_WriteReadBytes(HDC1080_I2C_CHANNEL, HDC1080_DEV_ADDR,
                                 HDC1080_REG_TEMPERATURE, 0U, NULL,
                                 HDC1080_READ_LEN, receive_data,
                                 HDC1080_MEAS_DELAY_MS);

        if (ret >= 0)
        {
            temp_x = (uint16_t)(((uint16_t)receive_data[0] << 8) |
                                 (uint16_t)receive_data[1]);
            humi_x = (uint16_t)(((uint16_t)receive_data[2] << 8) |
                                 (uint16_t)receive_data[3]);

            *temperature = (((float)temp_x / HDC1080_ADC_FULL_SCALE) *
                            HDC1080_TEMP_SPAN) - HDC1080_TEMP_OFFSET;
            *humidity    = ((float)humi_x / HDC1080_ADC_FULL_SCALE) *
                            HDC1080_HUMI_SPAN;

            ret = 0;   /* 읽은 바이트 수 → 성공 코드로 정규화 */
        }
    }

    return ret;
}
