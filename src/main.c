#include <stdio.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"

#include "vl53l0x_api.h"
#include "vl53.h"
#include "vl53l0x_api_calibration.h"

#define I2C_PORT        i2c0
#define I2C_SDA         4
#define I2C_SCL         5
#define VL53_GPIO1_PIN  6      
#define VL53_XSHUT_PIN  7
#define VL53_I2C_ADDR   0x29

#ifndef GPIO_IRQ_EDGE_FALL
#define GPIO_IRQ_EDGE_FALL 0x4u
#endif

static VL53L0X_Dev_t dev;
static volatile bool data_ready = false;

static inline void die_if(VL53L0X_Error st, const char *msg) {
    if (st) { printf("%s: %d\n", msg, st); while (1) tight_loop_contents(); }
}

static void gpio1_irq_handler(uint gpio, uint32_t events) {
    if (gpio == VL53_GPIO1_PIN) data_ready = true;
}

int main(void) {
    stdio_init_all();
    sleep_ms(200);
    printf("VL53L0X Continuous Ranging + GPIO1 interrupt (simplified)\n");

    // I2C @ 100 kHz
    i2c_init(I2C_PORT, 100 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);


    // Bring sensor out of reset (HIGH = enabled). If multiple sensors, hold all LOW, then enable one-by-one.
    vl53_xshut_set(VL53_XSHUT_PIN, false);  // ensure shutdown
    sleep_ms(5);
    vl53_xshut_set(VL53_XSHUT_PIN, true);   // power up
    sleep_ms(5);

    // GPIO1 interrupt input (active-low, falling edge)
    gpio_init(VL53_GPIO1_PIN);
    gpio_set_dir(VL53_GPIO1_PIN, GPIO_IN);
    gpio_pull_up(VL53_GPIO1_PIN);
    gpio_set_irq_enabled_with_callback(VL53_GPIO1_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio1_irq_handler);

    // Initialize VL53L0X sensor
    int result = vl53_init(&dev, VL53_I2C_ADDR);
    if (result != 0) {
        printf("VL53L0X initialization failed: %d\n", result);
        while (1) tight_loop_contents();
    }

    while (true) {
        if (data_ready) {
            data_ready = false;

            uint16_t distance;
            int result = vl53_read_mm(&dev, &distance);

            if (result == 0) {
                printf("Distance: %u mm\n", distance);
            } else {  
                printf("Range error: %d\n", result);
            }
        }
    }
}
