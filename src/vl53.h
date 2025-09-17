#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "hardware/i2c.h"
#include "vl53l0x_api.h"

// Initialization function
int vl53_init(VL53L0X_Dev_t *dev, uint8_t i2c_addr);

// GPIO control for XSHUT (power/reset)
void vl53_xshut_set(uint xshut_pin, bool enabled);
void vl53_xshut_low(void);
void vl53_xshut_high(void);

// Address change function
int vl53_set_address(VL53L0X_Dev_t *dev, uint8_t new_addr);

// Simple reading function for single sensor
int vl53_read_mm(VL53L0X_Dev_t *dev, uint16_t *mm);
/* XSHUT control
int  vl53_setup_xshut(uint pin);    // call once to choose the Pico pin that drives XSHUT
void vl53_xshut_low(void);          // hold sensor in hardware reset (XSHUT = 0)
void vl53_xshut_high(void);         // release reset (XSHUT = 1)
int  vl53_reset_and_reinit(void);   // pulse XSHUT and rerun the ST init sequence
*/

