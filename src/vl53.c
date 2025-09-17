#include <stdio.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "vl53l0x_api.h"
#include "vl53.h"
#include "vl53l0x_api_calibration.h"
#include "hardware/structs/io_bank0.h" 



static VL53L0X_Dev_t vl53_dev;
static uint s_xshut = 0xFF;           // 0xFF = not configured

void vl53_xshut_set(uint xshut_pin, bool enabled) {
    // XSHUT is active-LOW: LOW = shutdown, HIGH = run
    gpio_init(xshut_pin);
    gpio_set_dir(xshut_pin, GPIO_OUT);
    gpio_put(xshut_pin, enabled ? 1 : 0);
}

void vl53_xshut_low(void) {
    if (s_xshut != 0xFF) gpio_put(s_xshut, 0);
}

void vl53_xshut_high(void) {
    if (s_xshut != 0xFF) gpio_put(s_xshut, 1);
}

// Change the I²C address at runtime
// vl53.c
int vl53_set_address(VL53L0X_Dev_t *dev, uint8_t new_addr_7bit) {
    VL53L0X_StopMeasurement(dev);
    VL53L0X_ClearInterruptMask(dev, 0);
    sleep_ms(2);

    VL53L0X_SetDeviceAddress(dev, (uint8_t)(new_addr_7bit << 1));
    dev->I2cDevAddr = new_addr_7bit;

    VL53L0X_StartMeasurement(dev);
    return 0;
}

/////////////////////////////////

/*Xshut helper
int vl53_setup_xshut(uint pin) {
    s_xshut = pin;
    gpio_init(s_xshut);
    gpio_set_dir(s_xshut, GPIO_OUT);
    // default released (sensor ON)
    gpio_put(s_xshut, 1);
    sleep_ms(5);
    return 0;
}

void vl53_xshut_low(void) {
    if (s_xshut != 0xFF) gpio_put(s_xshut, 0);
}

void vl53_xshut_high(void) {
    if (s_xshut != 0xFF) gpio_put(s_xshut, 1);
}

int vl53_reset_and_reinit(void) {
    if (s_xshut == 0xFF) return -901; // not configured
    // hard reset pulse
    gpio_put(s_xshut, 0);
    sleep_ms(10);
    gpio_put(s_xshut, 1);
    sleep_ms(10);
    if (!s_last_i2c) return -902; // init was never called
    return vl53l0x_platform_init(s_last_freq_khz, s_last_sda, s_last_scl, s_last_i2c);
}
*/////////////////

static void vl53_i2c_init(int freq, int PIN_I2C_SDA,int PIN_I2C_SCL, i2c_inst_t *I2C_port) { // make it a parameter
    i2c_init(I2C_port, freq * 1000);  
    gpio_set_function(PIN_I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_I2C_SDA);
    gpio_pull_up(PIN_I2C_SCL);
}

int vl53_init(VL53L0X_Dev_t *dev, uint8_t i2c_addr) {
    if (!dev) return -1;
    
    // Set the I2C address
    dev->I2cDevAddr = i2c_addr;
    
    VL53L0X_Error st;
    
    // Basic initialization
    st = VL53L0X_DataInit(dev);
    if (st) {
        printf("VL53L0X DataInit failed: %d\n", st);
        return -100 - st;
    }
    
    st = VL53L0X_StaticInit(dev);
    if (st) {
        printf("VL53L0X StaticInit failed: %d\n", st);
        return -110 - st;
    }
    
    // Reference SPAD management
    uint32_t spad_count;
    uint8_t is_aperture;
    st = VL53L0X_PerformRefSpadManagement(dev, &spad_count, &is_aperture);
    if (st) {
        printf("VL53L0X RefSpadManagement failed: %d\n", st);
        return -120 - st;
    }
    
    // Reference calibration
    uint8_t vhv, phase;
    st = VL53L0X_PerformRefCalibration(dev, &vhv, &phase);
    if (st) {
        printf("VL53L0X RefCalibration failed: %d\n", st);
        return -130 - st;
    }
    
    // Set continuous ranging mode
    st = VL53L0X_SetDeviceMode(dev, VL53L0X_DEVICEMODE_CONTINUOUS_RANGING);
    if (st) {
        printf("VL53L0X SetDeviceMode failed: %d\n", st);
        return -140 - st;
    }
    
    // Configure GPIO for interrupt
    st = VL53L0X_SetGpioConfig(
        dev, 0,
        VL53L0X_DEVICEMODE_CONTINUOUS_RANGING,
        VL53L0X_GPIOFUNCTIONALITY_NEW_MEASURE_READY,
        VL53L0X_INTERRUPTPOLARITY_LOW
    );
    if (st) {
        printf("VL53L0X SetGpioConfig failed: %d\n", st);
        return -150 - st;
    }
    
    // Set timing budget
    st = VL53L0X_SetMeasurementTimingBudgetMicroSeconds(dev, 33000);
    if (st) {
        printf("VL53L0X SetTimingBudget failed: %d\n", st);
        return -160 - st;
    }
    
    // Start continuous measurement
    st = VL53L0X_StartMeasurement(dev);
    if (st) {
        printf("VL53L0X StartMeasurement failed: %d\n", st);
        return -170 - st;
    }
    
    printf("VL53L0X initialized successfully at address 0x%02X\n", i2c_addr);
    return 0;
}

int vl53_read_mm(VL53L0X_Dev_t *dev, uint16_t *mm) {
    if (!mm || !dev) return -1;

    VL53L0X_RangingMeasurementData_t m = {0};
    VL53L0X_Error st = VL53L0X_GetRangingMeasurementData(dev, &m);
    VL53L0X_ClearInterruptMask(dev, 0);
    
    if (st) {
        // Add debug info for common errors
        if (st == 20) {
            printf("DEBUG: I2C communication error\n");
        }
        return -200 - st;
    }

    if (m.RangeStatus != 0) {
        *mm = 0;
        return (int)m.RangeStatus;
    }

    *mm = (uint16_t)m.RangeMilliMeter;
    return 0;
}