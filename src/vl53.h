#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "hardware/i2c.h"
// pass freq in kHz, SDA pin, SCL pin, and which I2C peripheral (i2c0 or i2c1)
int vl53l0x_platform_init(int freq, int PIN_I2C_SDA,int PIN_I2C_SCL, i2c_inst_t *I2C_port);

int vl53_read_mm(uint16_t *mm);
int vl53_run_offset_cal_mm(uint16_t target_mm, int16_t *applied_mm);
int vl53_apply_fixed_offset_mm(int16_t offset_mm_mm);

// Configure GPIO1 (data-ready) pin once after init (pass your RP2040 pin)
int vl53_setup_gpio1(uint gpion);

// Non-blocking trigger: start one single measurement
int vl53_start_async(void);

// Optional: quick readiness check via the GPIO1 pin
bool vl53_ready_gpio(void);

// Read the finished measurement and clear the sensor interrupt
int vl53_read_async_mm(uint16_t *mm);
