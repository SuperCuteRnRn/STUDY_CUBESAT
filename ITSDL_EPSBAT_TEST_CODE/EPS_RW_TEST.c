#include "errno.h"
#include "fcntl.h"
#include "function.h"
#include "i2c_lib.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"

int32_t fd_txt;

void file_close() { close(fd_txt); }

uint8_t sw01_12v_PDM_STATE = 0;

int main(void) {
  printf("EPS_RW_TEST\n");

  sw01_12v_PDM_STATE = EPS_GetActualStateOfNPDM(0x01);
  printf("sw01_12v_PDM_STATE %d\n", sw01_12v_PDM_STATE);
  sleep(1);

  EPS_SwitchNPDMOn(0x01);
  printf("EPS_SwitchNPDMOn(1)\n");
  sleep(1);

  while (1) {
    sw01_12v_PDM_STATE = EPS_GetActualStateOfNPDM(0x01);
    printf("sw01_12v_PDM_STATE %d\n", sw01_12v_PDM_STATE);
    sleep(1);
  }

  return 0;
}