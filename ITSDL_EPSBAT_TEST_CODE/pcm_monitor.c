#include "errno.h"
#include "fcntl.h"
#include "function.h"
#include "i2c_lib.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"

// I2C 읽기 사이 딜레이 (마이크로초)
#define I2C_READ_DELAY_US 100000  // 100ms

int main(void) {
  double IPCM12V, VPCM12V;
  double IPCMBATV, VPCMBATV;
  double IPCM5V, VPCM5V;
  double IPCM3V3, VPCM3V3;

  printf("=== EPS PCM Voltage/Current Monitor ===\n");
  printf("Press Ctrl+C to exit\n\n");

  while (1) {
    // 12V Bus
    IPCM12V = EPS_GetTelemetry(EPS_TLE_IPCM12V) * 0.002066632361;  // A
    usleep(I2C_READ_DELAY_US);
    VPCM12V = EPS_GetTelemetry(EPS_TLE_VPCM12V) * 0.01349;         // V
    usleep(I2C_READ_DELAY_US);

    // Battery Bus
    IPCMBATV = EPS_GetTelemetry(EPS_TLE_IPCMBATV) * 0.00681988679; // A
    usleep(I2C_READ_DELAY_US);
    VPCMBATV = EPS_GetTelemetry(EPS_TLE_VPCMBATV) * 0.008978;      // V
    usleep(I2C_READ_DELAY_US);

    // 5V Bus
    IPCM5V = EPS_GetTelemetry(EPS_TLE_IPCM5V) * 0.00681988679;     // A
    usleep(I2C_READ_DELAY_US);
    VPCM5V = EPS_GetTelemetry(EPS_TLE_VPCM5V) * 0.005865;          // V
    usleep(I2C_READ_DELAY_US);

    // 3.3V Bus
    IPCM3V3 = EPS_GetTelemetry(EPS_TLE_IPCM3V3) * 0.00681988679;   // A
    usleep(I2C_READ_DELAY_US);
    VPCM3V3 = EPS_GetTelemetry(EPS_TLE_VPCM3V3) * 0.004311;        // V

    // 출력
    printf("----------------------------------------\n");
    printf("PCM 12V  : %.4f V, %.4f A\n", VPCM12V, IPCM12V);
    printf("PCM BATV : %.4f V, %.4f A\n", VPCMBATV, IPCMBATV);
    printf("PCM 5V   : %.4f V, %.4f A\n", VPCM5V, IPCM5V);
    printf("PCM 3V3  : %.4f V, %.4f A\n", VPCM3V3, IPCM3V3);
    printf("----------------------------------------\n\n");

    sleep(1);  // 1초 간격
  }

  return 0;
}

