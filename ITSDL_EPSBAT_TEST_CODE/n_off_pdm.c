#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "errno.h"
#include "stdint.h"
#include "fcntl.h"
#include "function.h"
#include "i2c_lib.h"

int main(void)
{
    char input[100];
    int indices[10];
    int count = 0;

    printf("Enter channel numbers to turn OFF (1-6, 8, 9) separated by spaces: ");
    fgets(input, sizeof(input), stdin);

    char *token = strtok(input, " ");
    while (token != NULL)
    {
        int channel = atoi(token);
        if (EPS_IsValidPDMChannel(channel))
        {
            indices[count++] = channel;
        }
        else
        {
            printf("Invalid channel number: %d. Valid: 1-6, 8, 9.\n", channel);
        }
        token = strtok(NULL, " ");
    }

    for (int i = 0; i < count; i++)
    {
        int index = indices[i];
        printf("index %d \r\n", index);
        /* 0x51 SET_PDM_N_OFF — 실제 스위치 OFF */
        EPS_SwitchNPDMOff(index);
        usleep(800000);
    }

    return 0;
}
