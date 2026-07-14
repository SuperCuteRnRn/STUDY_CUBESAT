#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "errno.h"
#include "stdint.h"
#include "fcntl.h"
#include "function.h"
#include "i2c_lib.h"

int32_t fd_txt;

void file_close()
{
    close(fd_txt);
}

int main(void)
{
    char buf[4086];
    fd_txt = open("./EPS_TLE_TEST.txt", O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    if (fd_txt < 0)
    {
        printf("File Open Error\n");
        return 0;
    }
    int index;
    uint16_t switch_flag;
    for (index = 1; index <= 10; index++)
    {
        if (!EPS_IsValidPDMChannel(index)) /* 지원 채널만 */
            continue;
        switch_flag = 0x0001;
        switch_flag = switch_flag << index;

        EPS_SetInitialStateOfNPDMOff(index);
        printf("test %d \r\n", index);
        usleep(800000);
    }
    file_close();
}