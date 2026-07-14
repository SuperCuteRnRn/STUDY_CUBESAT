#include "stdio.h"
#include "unistd.h"
#include "i2c_lib.h"
#include "function.h"

/******************************************************************************
**  Function:  BAT_SetHeaterStatus()
**
**  Purpose:
**      Battery의 Heater 상태를 설정하는 함수.
**          0x00 : Thermostat control circuitry disabled.
**          0x01 : Thermostat control circuitry enabled.
*/

int main()
{
    uint16_t test;
    test = EPS_GetNumberOfBrownOutReset();
    printf("Brownout(IWDG) resets  %02X\n", test);
    usleep(500000);
    test = EPS_GetNumberOfAutomaticReset();
    printf("Auto SW resets         %02X\n", test);
    usleep(500000);
    test = EPS_GetNumberOfManuelReset();
    printf("Manual(PIN) resets     %02X\n", test);
    usleep(500000);
    test = EPS_GetNumberOfCommWatchdogReset();
    printf("Power-on(POR) resets   %02X\n", test);
    usleep(500000);

test = EPS_GetActualStateOfAllPDM();
    printf("EPS_GetActualStateOfAllPDM %02X\n", test);
usleep(500000);
test = EPS_GetExpectedStateOfAllPDM();
    printf("EPS_GetExpectedStateOfAllPDM %02X\n", test);
usleep(500000);
test = EPS_GetInitialStateOfAllPDM();
    printf("EPS_GetInitialStateOfAllPDM %02X\n", test);
usleep(500000);


    return 0;
}