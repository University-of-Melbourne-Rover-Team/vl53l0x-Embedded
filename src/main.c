#include <stdio.h>
#include "pico/stdlib.h"
#include "vl53.h"

#ifndef VL53_GPIO1_PIN
#define VL53_GPIO1_PIN 6   // <-- RP2040 pin wired to VL53L0X GPIO1 (active-LOW)
#endif
//test
int main() {
    stdio_init_all();
    sleep_ms(1000);

    int rc = vl53l0x_platform_init(100, 4, 5, i2c0);
    if (rc != 0) {
        printf("VL53 init failed: %d\n", rc);
        while (1) sleep_ms(1000);
    }
    printf("VL53 init OK\n");

    vl53_setup_gpio1(VL53_GPIO1_PIN);

    int16_t applied = 0;
    rc = vl53_run_offset_cal_mm(100, &applied);   // target at 100 mm
    if (rc == 0) {
        printf("Calibration applied: %d mm offset\n", applied);
    } else {
        printf("Calibration failed: %d\n", rc);
    }

    printf("\nControls:\n");
    printf("  [space]  blocking measurement (original)\n");
    printf("  [q]      start non-blocking measurement (no wait)\n");
    printf("  [w]      read non-blocking result\n\n");

    while (true) {
        int ch = getchar();  // wait for key
        if (ch == ' ') {
            // Report current GPIO readiness before the blocking read
            if (vl53_ready_gpio()) {
                printf("[SPACE] GPIO says: data ready\n");
            } else {
                printf("[SPACE] GPIO says: data not ready\n");
            }

            uint16_t mm = 0;
            int r = vl53_read_mm(&mm);   // blocking: starts & waits internally
            if (r == 0) {
                printf("[SPACE] Distance: %u mm\n", mm);
            } else if (r > 0) {
                printf("[SPACE] VL53 RangeStatus: %d\n", r);
            } else {
                printf("[SPACE] VL53 read error: %d\n", r);
            }
        } else if (ch == 'q' || ch == 'Q') {
            // --- Start non-blocking measurement ---
            int r = vl53_start_async();
            if (r == 0) {
                // Immediately report readiness state after kicking it off
                if (vl53_ready_gpio()) {
                    printf("[Q] started; GPIO says: data ready\n");
                } else {
                    printf("[Q] started; GPIO says: data not ready\n");
                }
            } else {
                printf("[Q] start error: %d\n", r);
            }
        } else if (ch == 'w' || ch == 'W') {
            // --- Read non-blocking result ---
            if (vl53_ready_gpio()) {
                printf("[W] GPIO says: data ready\n");
            } else {
                printf("[W] GPIO says: data not ready\n");
            }

            uint16_t mm = 0;
            int r = vl53_read_async_mm(&mm);
            if (r == 0) {
                printf("[W] Distance: %u mm\n", mm);
            } else if (r > 0) {
                printf("[W] VL53 RangeStatus: %d\n", r);
            } else {
                printf("[W] VL53 read error: %d\n", r);
            }
        } else {
            printf("Press [space]=blocking, [q]=start NB, [w]=read NB\n");
        }
    }
}
