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

int vl53_read_mm(VL53L0X_Dev_t *dev, uint16_t *mm, uint8_t i2c_addr) {
    if (!mm || !dev) return -1;

    // Temporarily store the original address
    uint8_t original_addr = dev->I2cDevAddr;
    
    // Set the device to use the specified address
    dev->I2cDevAddr = i2c_addr;

    VL53L0X_RangingMeasurementData_t m = {0};
    VL53L0X_Error st = VL53L0X_GetRangingMeasurementData(dev, &m);
    VL53L0X_ClearInterruptMask(dev, 0);
    
    // Restore the original address
    dev->I2cDevAddr = original_addr;
    
    if (st) {
        // Add debug info for common errors
        if (st == 20) {
            printf("DEBUG: No sensor responding at address 0x%02X (I2C error)\n", i2c_addr);
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