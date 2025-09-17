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
#define VL53_I2C_ADDR_DEFAULT  0x29
#define VL53_I2C_ADDR_ALT      0x30

#ifndef GPIO_IRQ_EDGE_FALL
#define GPIO_IRQ_EDGE_FALL 0x4u
#endif

static VL53L0X_Dev_t dev;
static volatile bool data_ready = false;
static uint8_t current_address = VL53_I2C_ADDR_DEFAULT;

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

    // VL53L0X init + calibration
    dev.I2cDevAddr = VL53_I2C_ADDR_DEFAULT;

    VL53L0X_Error st;
    st = VL53L0X_DataInit(&dev);                                 die_if(st, "DataInit failed");
    st = VL53L0X_StaticInit(&dev);                               die_if(st, "StaticInit failed");

    uint32_t spad_count; uint8_t is_aperture;
    st = VL53L0X_PerformRefSpadManagement(&dev, &spad_count, &is_aperture);
    die_if(st, "RefSpadManagement failed");

    uint8_t vhv, phase;
    st = VL53L0X_PerformRefCalibration(&dev, &vhv, &phase);      die_if(st, "RefCalibration failed");

    // Continuous mode + "new sample ready" on GPIO1 (active-low)
    st = VL53L0X_SetDeviceMode(&dev, VL53L0X_DEVICEMODE_CONTINUOUS_RANGING);
    die_if(st, "SetDeviceMode failed");

    st = VL53L0X_SetGpioConfig(
        &dev, 0,
        VL53L0X_DEVICEMODE_CONTINUOUS_RANGING,
        VL53L0X_GPIOFUNCTIONALITY_NEW_MEASURE_READY,
        VL53L0X_INTERRUPTPOLARITY_LOW
    );                                                             die_if(st, "SetGpioConfig failed");

    // ~33 ms timing budget (adjust as needed)
    st = VL53L0X_SetMeasurementTimingBudgetMicroSeconds(&dev, 33000);
    die_if(st, "SetTimingBudget failed");

    st = VL53L0X_StartMeasurement(&dev);                          die_if(st, "StartMeasurement failed");
    printf("Continuous measurement started.\n");
    printf("Controls:\n");
    printf("  Press '1' to read from address 0x%02X\n", VL53_I2C_ADDR_DEFAULT);
    printf("  Press '2' to read from address 0x%02X\n", VL53_I2C_ADDR_ALT);
    printf("  Press SPACE to change a sensor's address\n");
    printf("Current reading address: 0x%02X\n", current_address);

    while (true) {
        if (data_ready) {
            data_ready = false;

            uint16_t distance;
            int result = vl53_read_mm(&dev, &distance, current_address);

            if (result == 0) {
                printf("Distance (0x%02X): %u mm\n", current_address, distance);
            } else {  
                printf("Range error from sensor 0x%02X: %d\n", current_address, result);
            }
        }
        // Check for keyboard input
        int c = getchar_timeout_us(0);  // non-blocking getchar

        if (c == '1') {
            current_address = VL53_I2C_ADDR_DEFAULT;
            printf("Switched to reading from address 0x%02X\n", current_address);
        } else if (c == '2') {
            current_address = VL53_I2C_ADDR_ALT;
            printf("Switched to reading from address 0x%02X\n", current_address);
        } else if (c == ' ') {
            printf("[Main] SPACE pressed: toggling XSHUT + changing address\n");
            // Toggle sensor reset
            vl53_xshut_low();
            sleep_ms(5);

            // Change address (example: 0x30)
            if (vl53_set_address(&dev, 0x30) == 0) {
                printf("[Main] Address successfully changed to 0x30\n");
            } else {
                printf("[Main] Failed to change address\n");
            }
            
            vl53_xshut_high();
            sleep_ms(5);
        }


    }
}
