#include "hdc1080.h"
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    float temperature = 0.0f;
    float humidity = 0.0f;
    int   ret = 0;

    ret = hdc1080_init(Temperature_Resolution_14_bit,
                       Humidity_Resolution_14_bit);
    if (ret != 0)
    {
        (void)printf("HDC1080 init failed (%d)\n", ret);
        return 1;
    }

    while (1)
    {
        if (hdc1080_start_measurement(&temperature, &humidity) == 0)
        {
            (void)printf("Temperature: %.2f C\n", (double)temperature);
            (void)printf("Humidity:    %.2f %%\n", (double)humidity);
        }
        (void)usleep(100000U);
    }

    return 0;
}
