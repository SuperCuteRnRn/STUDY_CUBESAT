#include "errno.h"
#include "fcntl.h"
#include "function.h"
#include "i2c_lib.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"

// #define _NO_TXT

int32_t fd_txt;

void file_close() { close(fd_txt); }

int main(void) {
  char buf[4086];

#ifdef _NO_TXT
  fd_txt = open("./EPS_TLE_TEST.txt", O_RDWR | O_CREAT | O_APPEND,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

  if (fd_txt < 0) {
    printf("File Open Error\n");
    return 0;
  }
#endif
  /////////////////////////////////////////////////////

  double IPCM12V;
  double VPCM12V;

  double IPCMBATV;
  double VPCMBATV;

  double IPCM5V;
  double VPCM5V;

  double IPCM3V3;
  double VPCM3V3;

  double VSW[11];
  double ISW[11];

  double TBRD;

  ////////////////////////////////////////////////////////////////////////////////
  double TBRD_DB;
  //////////////////////////////////////////////////////////////////////////////////////////

#ifdef BAT
  double TLM_VBAT;
  double TLM_IBAT;
  double TLM_IDIRBAT;
  double TLM_TBRD;
  double TLM_IPCM5V;
  double TLM_VPCM5V;
  double TLM_IPCM3V3;
  double TLM_VPCM3V3;
  double TLM_TBAT[4];
  double TLM_HBAT[4];
#endif
#ifdef _NO_TXT

#ifdef BAT
  TLM_VBAT = BAT_GetTelemetry(BAT_TLM_VBAT) * 0.008993;  // V
  TLM_IBAT = BAT_GetTelemetry(BAT_TLM_IBAT) * 14.662757; // mA
  TLM_IDIRBAT = BAT_GetTelemetry(BAT_TLM_IDIRBAT);
  TLM_TBRD = (BAT_GetTelemetry(BAT_TLM_TBRD) * 0.3727434) - 273.15; //'C
  TLM_IPCM5V = BAT_GetTelemetry(BAT_TLM_IPCM5V) * 1.327547;         // mA
  TLM_VPCM5V = BAT_GetTelemetry(BAT_TLM_VPCM5V) * 0.005865;         // V
  TLM_IPCM3V3 = BAT_GetTelemetry(BAT_TLM_IPCM3V3) * 1.327547;       // mA
  TLM_VPCM3V3 = BAT_GetTelemetry(BAT_TLM_VPCM3V3) * 0.004311;       // V

  TLM_TBAT[0] = (BAT_GetTelemetry(BAT_TLM_TBAT1) * 0.397600) - 273.15; //'C
  TLM_TBAT[1] = (BAT_GetTelemetry(BAT_TLM_TBAT2) * 0.397600) - 273.15; //'C
  TLM_TBAT[2] = (BAT_GetTelemetry(BAT_TLM_TBAT3) * 0.397600) - 273.15; //'C
  TLM_TBAT[3] = (BAT_GetTelemetry(BAT_TLM_TBAT4) * 0.397600) - 273.15; //'C

  TLM_HBAT[0] = BAT_GetTelemetry(BAT_TLM_HBAT1);
  TLM_HBAT[1] = BAT_GetTelemetry(BAT_TLM_HBAT2);
  TLM_HBAT[2] = BAT_GetTelemetry(BAT_TLM_HBAT3);
  TLM_HBAT[3] = BAT_GetTelemetry(BAT_TLM_HBAT4);
#endif

  /////////////////////////////////////////////////////////////

  ////////////////////////////////////////////////////////////////
  IPCM12V = EPS_GetTelemetry(EPS_TLE_IPCM12V) * 0.002066632361; // A
  printf("IPCM12V : %f\n", IPCM12V);
  VPCM12V = EPS_GetTelemetry(EPS_TLE_VPCM12V) * 0.01349; // V
  printf("VPCM12V : %f\n", VPCM12V);
  IPCMBATV = EPS_GetTelemetry(EPS_TLE_IPCMBATV) * 0.00681988679; // A
  printf("IPCMBATV : %f\n", IPCMBATV);
  VPCMBATV = EPS_GetTelemetry(EPS_TLE_VPCMBATV) * 0.008978; // V
  printf("VPCMBATV : %f\n", VPCMBATV);

  IPCM5V = EPS_GetTelemetry(EPS_TLE_IPCM5V) * 0.00681988679; // A
  printf("IPCM5V : %f\n", IPCM5V);
  VPCM5V = EPS_GetTelemetry(EPS_TLE_VPCM5V) * 0.005865; // V
  printf("VPCM5V : %f\n", VPCM5V);
  IPCM3V3 = EPS_GetTelemetry(EPS_TLE_IPCM3V3) * 0.00681988679; // A
  printf("IPCM3V3 : %f\n", IPCM3V3);
  VPCM3V3 = EPS_GetTelemetry(EPS_TLE_VPCM3V3) * 0.004311; // V
  printf("VPCM3V3 : %f\n", VPCM3V3);

  VSW[0] = EPS_GetTelemetry(EPS_TLE_VSW1) * 0.01349;
  printf("VSW[0] : %f\n", VSW[0]);
  VSW[1] = EPS_GetTelemetry(EPS_TLE_VSW2) * 0.01349;
  printf("VSW[1] : %f\n", VSW[1]);
  VSW[2] = EPS_GetTelemetry(EPS_TLE_VSW3) * 0.008993;
  printf("VSW[2] : %f\n", VSW[2]);
  VSW[3] = EPS_GetTelemetry(EPS_TLE_VSW4) * 0.008993;
  printf("VSW[3] : %f\n", VSW[3]);
  VSW[4] = EPS_GetTelemetry(EPS_TLE_VSW5) * 0.005865;
  printf("VSW[5] : %f\n", VSW[5]);
  VSW[6] = EPS_GetTelemetry(EPS_TLE_VSW6) * 0.005865;
  printf("VSW[6] : %f\n", VSW[6]);
  VSW[8] = EPS_GetTelemetry(EPS_TLE_VSW8) * 0.004311;
  printf("VSW[8] : %f\n", VSW[8]);
  VSW[9] = EPS_GetTelemetry(EPS_TLE_VSW9) * 0.004311;
  printf("VSW[9] : %f\n", VSW[9]);

  ISW[0] = EPS_GetTelemetry(EPS_TLE_ISW1) * 0.001328;
  printf("ISW[1] : %f\n", ISW[0]);
  ISW[1] = EPS_GetTelemetry(EPS_TLE_ISW2) * 0.001328;
  printf("ISW[2] : %f\n", ISW[1]);
  ISW[2] = EPS_GetTelemetry(EPS_TLE_ISW3) * 0.006239;
  printf("ISW[3] : %f\n", ISW[2]);
  ISW[3] = EPS_GetTelemetry(EPS_TLE_ISW4) * 0.006239;
  printf("ISW[4] : %f\n", ISW[3]);
  ISW[4] = EPS_GetTelemetry(EPS_TLE_ISW5) * 0.001328;
  printf("ISW[5] : %f\n", ISW[4]);
  ISW[5] = EPS_GetTelemetry(EPS_TLE_ISW6) * 0.001328;
  printf("ISW[6] : %f\n", ISW[5]);
  ISW[7] = EPS_GetTelemetry(EPS_TLE_ISW8) * 0.001328;
  printf("ISW[8] : %f\n", ISW[8]);
  ISW[9] = EPS_GetTelemetry(EPS_TLE_ISW9) * 0.001328;
  printf("ISW[9] : %f\n", ISW[9]);

  TBRD = (EPS_GetTelemetry(EPS_TLE_TBRD) * 0.372434) - 273.15;
  printf("TBRD : %f\n", TBRD);
  ///////////////////////////////////////////////////////////////////////////////////////////////








  TBRD_DB = (EPS_GetTelemetry(EPS_TLE_TLM_TBRD_DB) * 0.372434) - 273.15;
  printf("TBRD_DB : %f\n", TBRD_DB);
///////////////////////////////////////////////////////////////////////////////////////////////////////////
#else
  while (1) {
#ifdef BAT
    TLM_VBAT = BAT_GetTelemetry(BAT_TLM_VBAT) * 0.008993;  // V
    TLM_IBAT = BAT_GetTelemetry(BAT_TLM_IBAT) * 14.662757; // mA
    TLM_IDIRBAT = BAT_GetTelemetry(BAT_TLM_IDIRBAT);
    TLM_TBRD = (BAT_GetTelemetry(BAT_TLM_TBRD) * 0.3727434) - 273.15; //'C
    TLM_IPCM5V = BAT_GetTelemetry(BAT_TLM_IPCM5V) * 1.327547;         // mA
    TLM_VPCM5V = BAT_GetTelemetry(BAT_TLM_VPCM5V) * 0.005865;         // V
    TLM_IPCM3V3 = BAT_GetTelemetry(BAT_TLM_IPCM3V3) * 1.327547;       // mA
    TLM_VPCM3V3 = BAT_GetTelemetry(BAT_TLM_VPCM3V3) * 0.004311;       // V

    TLM_TBAT[0] = (BAT_GetTelemetry(BAT_TLM_TBAT1) * 0.397600) - 273.15; //'C
    TLM_TBAT[1] = (BAT_GetTelemetry(BAT_TLM_TBAT2) * 0.397600) - 273.15; //'C
    TLM_TBAT[2] = (BAT_GetTelemetry(BAT_TLM_TBAT3) * 0.397600) - 273.15; //'C
    TLM_TBAT[3] = (BAT_GetTelemetry(BAT_TLM_TBAT4) * 0.397600) - 273.15; //'C

    TLM_HBAT[0] = BAT_GetTelemetry(BAT_TLM_HBAT1);
    TLM_HBAT[1] = BAT_GetTelemetry(BAT_TLM_HBAT2);
    TLM_HBAT[2] = BAT_GetTelemetry(BAT_TLM_HBAT3);
    TLM_HBAT[3] = BAT_GetTelemetry(BAT_TLM_HBAT4);
#endif

    /////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////
    sleep(1);
    sleep(1);
    sleep(1);
    sleep(1);
    IPCM12V = EPS_GetTelemetry(EPS_TLE_IPCM12V) * 0.002066632361; // A
    sleep(1);
    VPCM12V = EPS_GetTelemetry(EPS_TLE_VPCM12V) * 0.01349;        // V
    sleep(1);
    IPCMBATV = EPS_GetTelemetry(EPS_TLE_IPCM12V) * 0.00681988679; // A
    sleep(1);
    VPCMBATV = EPS_GetTelemetry(EPS_TLE_VPCMBATV) * 0.005865;     // V
    sleep(1);
    IPCM5V = EPS_GetTelemetry(EPS_TLE_IPCM5V) * 0.00681988679;    // A
    sleep(1);
    VPCM5V = EPS_GetTelemetry(EPS_TLE_VPCM5V) * 0.005865;         // V
    sleep(1);
    IPCM3V3 = EPS_GetTelemetry(EPS_TLE_IPCM3V3) * 0.00681988679;  // A
    sleep(1);
    VPCM3V3 = EPS_GetTelemetry(EPS_TLE_VPCM3V3) * 0.005865;       // V
sleep(1);
    VSW[0] = EPS_GetTelemetry(EPS_TLE_VSW1) * 0.01349;
    sleep(1);
    VSW[1] = EPS_GetTelemetry(EPS_TLE_VSW2) * 0.01349;
    sleep(1);
    VSW[2] = EPS_GetTelemetry(EPS_TLE_VSW3) * 0.01349;
    sleep(1);
    VSW[3] = EPS_GetTelemetry(EPS_TLE_VSW4) * 0.01349;
    sleep(1);
    VSW[4] = EPS_GetTelemetry(EPS_TLE_VSW5) * 0.01349;
    sleep(1);
    VSW[5] = EPS_GetTelemetry(EPS_TLE_VSW6) * 0.01349;
    sleep(1);
    sleep(1);
    VSW[7] = EPS_GetTelemetry(EPS_TLE_VSW8) * 0.01349;
    sleep(1);
    VSW[8] = EPS_GetTelemetry(EPS_TLE_VSW9) * 0.01349;
    sleep(1);
sleep(1);
    ISW[0] = EPS_GetTelemetry(EPS_TLE_ISW1) * 0.001328;
    sleep(1);
    ISW[1] = EPS_GetTelemetry(EPS_TLE_ISW2) * 0.001328;
      sleep(1);
    ISW[2] = EPS_GetTelemetry(EPS_TLE_ISW3) * 0.001328;
    sleep(1);
    ISW[3] = EPS_GetTelemetry(EPS_TLE_ISW4) * 0.001328;
    sleep(1);
    ISW[4] = EPS_GetTelemetry(EPS_TLE_ISW5) * 0.001328;
    sleep(1);
    ISW[5] = EPS_GetTelemetry(EPS_TLE_ISW6) * 0.001328;
    sleep(1);
    sleep(1);
    ISW[7] = EPS_GetTelemetry(EPS_TLE_ISW8) * 0.001328;
    sleep(1);
    ISW[8] = EPS_GetTelemetry(EPS_TLE_ISW9) * 0.001328;
    sleep(1);
    sleep(1);

    TBRD = (EPS_GetTelemetry(EPS_TLE_TBRD) * 0.372434) - 273.15;
    ///////////////////////////////////////////////////////////////////////////////////////////////
sleep(1);
    sleep(1);
    sleep(1);
    sleep(1);
    sleep(1);
    sleep(1);
    sleep(1);
    sleep(1);
    sleep(1);
    sleep(1);

    sleep(1);
    sleep(1);
    sleep(1);
    
    sleep(1);
    sleep(1);
    sleep(1);

    sleep(1);
    sleep(1);
    sleep(1);
    sleep(1);
    sleep(1);
    sleep(1);
    sleep(1);
    sleep(1);

    sleep(1);
    sleep(1);
    sleep(1);
    sleep(1);
    sleep(1);
    sleep(1);
    sleep(1);
    sleep(1);
    sleep(1);

    sleep(1);
    sleep(1);
    sleep(1);
    sleep(1);
    sleep(1);
    sleep(1);
    sleep(1);
    sleep(1);
    sleep(1);

    sleep(1);
    sleep(1);
    sleep(1);
    sleep(1);
    sleep(1);
    sleep(1);
    
    sleep(1);
    
    sleep(1);
    sleep(1);

    sleep(1);
    sleep(1);
    
    sleep(1);
    sleep(1);
    sleep(1);
    sleep(1);
    sleep(1);
    sleep(1);
    sleep(1);
sleep(1);
    TBRD_DB = (EPS_GetTelemetry(EPS_TLE_TLM_TBRD_DB) * 0.372434) - 273.15; //'C
    /* 지원 텔레메트리만 CSV 한 줄로 출력 (BCR/SW7/SW10 제거됨) */
    memset(&buf, 0, sizeof(buf));
    sprintf(buf,
            "%f A, %f V, %f A, %f V, %f A, %f V, %f A, %f V, "
            "%f %f %f %f %f %f %f %f V(SW), "
            "%f %f %f %f %f %f %f %f A(SW), "
            "%f 'C, %f 'C\n",
            IPCM12V, VPCM12V, IPCMBATV, VPCMBATV, IPCM5V, VPCM5V, IPCM3V3,
            VPCM3V3, VSW[0], VSW[1], VSW[2], VSW[3], VSW[4], VSW[5], VSW[7],
            VSW[8], ISW[0], ISW[1], ISW[2], ISW[3], ISW[4], ISW[5], ISW[7],
            ISW[8], TBRD, TBRD_DB);
    printf("%s", buf);

#ifdef BAT

    memset(&buf, 0, sizeof(buf));
    sprintf(buf,
            " TLM_VBAT , TLM_IBAT , TLM_IDIRBAT , TLM_TBRD , TLM_IPCM5V , "
            "TLM_VPCM5V, TLM_IPCM3V3 , TLM_VPCM3V3, TLM_TBAT[0] , TLM_TBAT[1]"
            ",TLM_TBAT[2] , TLM_TBAT[3] , TLM_HBAT[0] , TLM_HBAT[1] , "
            "TLM_HBAT[2] , TLM_HBAT[3] \n");
    memset(&buf, 0, sizeof(buf));
    sprintf(buf,
            " %02f V , %02f mA , %02f  , %02f 'C , %02f mA , %02f V , %02f mA "
            ", %02f V , %02f 'C , %02f  ,"
            " %02f 'C , %02f , %02f 'C , %02f , %02f 'C , %02f ,",
            TLM_VBAT, TLM_IBAT, TLM_IDIRBAT, TLM_TBRD, TLM_IPCM5V, TLM_VPCM5V,
            TLM_IPCM3V3, TLM_VPCM3V3, TLM_TBAT[0], TLM_TBAT[1], TLM_TBAT[2],
            TLM_TBAT[3], TLM_HBAT[0], TLM_HBAT[1], TLM_HBAT[2], TLM_HBAT[3]);

#endif
    sleep(10000);
  }

#endif

#ifdef _NO_TXT

  memset(&buf, 0, sizeof(buf));
               ",VPCMBATV, IPCM12V , VPCM12V, IPCM5V , VPCM5V"
               ",IPCM3V3 , VPCM3V3 , VSW[0] , VSW[1] , VSW[2] , VSW[3] , "
               "VSW[4] , VSW[5] , VSW[6] , VSW[7]"
               ",VSW[8] , VSW[9], ISW[0] , ISW[1] , ISW[2] , ISW[3] , ISW[4] , "
               "ISW[5] , ISW[6] , ISW[7] "
  write(fd_txt, buf, strlen(buf));

  memset(&buf, 0, sizeof(buf));
  sprintf(buf,
          " %02f A , %02f V , %02f A , %02f A , %02f A , %02f V , %02f A , "
          "%02f V , %02f A , %02f V ,"
          " %02f A , %02f V , %02f V , %02f V , %02f V , %02f V , %02f V , "
          "%02f V , %02f V , %02f V ,"
          " %02f V , %02f V , %02f A , %02f A , %02f A , %02f A , %02f A , "
          "%02f A , %02f A , %02f A ,"
          " %02f 'C , %02f V , %02f V , %02f V , %02f V , %02f V , %02f V , "
          "%02f V , %02f V , %02f V ,"
          " %02f A , %02f A , %02f A , %02f A , %02f A , %02f A , %02f A , "
          "%02f A , %02f A , %02f A ,"
          " %02f A , %02f A , %02f A , %02f A , %02f A , %02f A , %02f A , "
          "%02f A , %02f 'C , %02f 'C ,"
          " %02f 'C , %02f 'C , %02f 'C , %02f 'C , %02f 'C , %02f 'C , %02f "
          "'C , %02f 'C , %02f 'C ,"
          " %02f 'C , %02f 'C , %02f 'C , %02f 'C , %02f 'C , %02f 'C , %02f "
          "'C , %02f 'C , %02f 'C ,"
          " %02f 'C , %02f 'C , %02f 'C , %02f 'C , %02f 'C , %02f 'C , %02f "
          "W/m^2 , %02f W/m^2 , %02f W/m^2 , %02f W/m^2 ,"
          " %02f W/m^2 , %02f W/m^2 , %02f W/m^2 , %02f W/m^2 , %02f W/m^2 , "
          "%02f W/m^2 , %02f W/m^2 , %02f W/m^2 , %02f W/m^2 , %02f W/m^2 ,"
          " %02f W/m^2 , %02f W/m^2 , %02f W/m^2 , %02f W/m^2 ,",
          IPCM12V, VPCM12V, IPCM5V, VPCM5V, IPCM3V3, VPCM3V3, VSW[0], VSW[1],
          VSW[2], VSW[3], VSW[4], VSW[5], VSW[6], VSW[7], VSW[8], VSW[9],
          ISW[0], ISW[1], ISW[2], ISW[3], ISW[4], ISW[5], ISW[6], ISW[7],
  write(fd_txt, buf, strlen(buf));

#ifdef BAT

  memset(&buf, 0, sizeof(buf));
  sprintf(buf,
          " TLM_VBAT , TLM_IBAT , TLM_IDIRBAT , TLM_TBRD , TLM_IPCM5V , "
          "TLM_VPCM5V, TLM_IPCM3V3 , TLM_VPCM3V3, TLM_TBAT[0] , TLM_TBAT[1]"
          ",TLM_TBAT[2] , TLM_TBAT[3] , TLM_HBAT[0] , TLM_HBAT[1] , "
          "TLM_HBAT[2] , TLM_HBAT[3] \n");
  write(fd_txt, buf, strlen(buf));

  memset(&buf, 0, sizeof(buf));
  sprintf(buf,
          " %02f V , %02f mA , %02f  , %02f 'C , %02f mA , %02f V , %02f mA , "
          "%02f V , %02f 'C , %02f  ,"
          " %02f 'C , %02f , %02f 'C , %02f , %02f 'C , %02f ,",
          TLM_VBAT, TLM_IBAT, TLM_IDIRBAT, TLM_TBRD, TLM_IPCM5V, TLM_VPCM5V,
          TLM_IPCM3V3, TLM_VPCM3V3, TLM_TBAT[0], TLM_TBAT[1], TLM_TBAT[2],
          TLM_TBAT[3], TLM_HBAT[0], TLM_HBAT[1], TLM_HBAT[2], TLM_HBAT[3]);
  write(fd_txt, buf, strlen(buf));

#endif

  file_close();
#endif
}