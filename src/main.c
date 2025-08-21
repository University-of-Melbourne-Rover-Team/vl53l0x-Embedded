#include <stdio.h>
#include "pico/stdlib.h"
#include "vl53.h"

int main() {
    stdio_init_all();
    sleep_ms(1000); 

    int rc = vl53l0x_platform_init();
    if (rc != 0) {
        printf("VL53 init failed: %d\n", rc);
        while (1) sleep_ms(1000);
    }
    printf("VL53 init OK\n");
    int16_t applied = 0;
    rc = vl53_run_offset_cal_mm(100, &applied);   // target at 100 mm
    if (rc == 0) {
        printf("Calibration applied: %d mm offset\n", applied);
    } else {
        printf("Calibration failed: %d\n", rc);
    }

    while (true) {
        printf("Press ENTER (or any key) to take a measurement.\n");
        int ch = getchar(); 
        (void)ch;         
        uint16_t mm = 0;
        int r = vl53_read_mm(&mm);
        if (r == 0) {
            printf("Distance: %u mm\n", mm);
        } else if (r > 0) {
            printf("VL53 RangeStatus: %d\n", r);
        } else {
            printf("VL53 read error: %d\n", r);
        }
    }
}
