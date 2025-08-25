#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "vl53l0x_api.h"
#include "vl53.h"
#include "vl53l0x_api_calibration.h"



static uint s_gpio1 = 0xFF;   // 0xFF == not set
static VL53L0X_Dev_t vl53_dev;

static void sensor_hard_reset(void) {
#ifdef PIN_XSHUT
    gpio_init(PIN_XSHUT);
    gpio_set_dir(PIN_XSHUT, GPIO_OUT);
    gpio_put(PIN_XSHUT, 0);
    sleep_ms(10);
    gpio_put(PIN_XSHUT, 1);
    sleep_ms(10);
#else
#endif
}
//////////////////////////////////////////////////////////
int vl53_setup_gpio1(uint gpion) {
    s_gpio1 = gpion;
    gpio_init(s_gpio1);
    gpio_set_dir(s_gpio1, GPIO_IN);
    gpio_pull_up(s_gpio1); 
    return 0;
}

bool vl53_ready_gpio(void) {
    if (s_gpio1 == 0xFF) return false;  // not configured
    return gpio_get(s_gpio1) == 0;      // active-LOW means 0 = ready
}
// Non-blocking trigger: start one single measurement
int vl53_start_async(void) {
    VL53L0X_Error st = VL53L0X_StartMeasurement(&vl53_dev);
    return st ? (-300 - st) : 0;
}

// Read the finished measurement and clear the sensor interrupt
int vl53_read_async_mm(uint16_t *mm) {
    if (!mm) return -1;

    VL53L0X_RangingMeasurementData_t m = {0};
    VL53L0X_Error st = VL53L0X_GetRangingMeasurementData(&vl53_dev, &m);
    if (st) return -200 - st;

    VL53L0X_ClearInterruptMask(&vl53_dev, 0);

    if (m.RangeStatus != 0) {
        *mm = 0;
        return (int)m.RangeStatus; 
    }

    *mm = (uint16_t)m.RangeMilliMeter;
    return 0;
}

/////////////////////////////////

static void vl53_i2c_init(int freq, int PIN_I2C_SDA,int PIN_I2C_SCL, i2c_inst_t *I2C_port) { // make it a parameter
    i2c_init(I2C_port, freq * 1000);  
    gpio_set_function(PIN_I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_I2C_SDA);
    gpio_pull_up(PIN_I2C_SCL);
}
int vl53_run_offset_cal_mm(uint16_t target_mm, int16_t *applied_mm) {
    FixPoint1616_t cal_dist_q16 = ((FixPoint1616_t)target_mm) << 16;
    int32_t offset_um = 0;

    VL53L0X_Error st = VL53L0X_perform_offset_calibration(&vl53_dev,
                              cal_dist_q16, &offset_um);
    if (st) return -310 - st;

    if (applied_mm) *applied_mm = (int16_t)(offset_um / 1000);
    return 0;
}
int vl53l0x_platform_init(int freq, int PIN_I2C_SDA,int PIN_I2C_SCL, i2c_inst_t *I2C_port) {
    VL53L0X_Error st;
    uint32_t spad_count = 0;
    uint8_t is_aperture = 0;
    uint8_t vhv = 0, phase = 0;

    vl53_i2c_init(freq, PIN_I2C_SDA, PIN_I2C_SCL, I2C_port);
    sensor_hard_reset();
    vl53_dev.I2cDevAddr = 0x29; //write another function to do this with a parameter
                // provide it with XSHUT PIN and dev and addr
    sleep_ms(10); 

    st = VL53L0X_DataInit(&vl53_dev);
    if (st) { printf("DataInit=%d\n", st); return -100 - st; }

    st = VL53L0X_StaticInit(&vl53_dev);
    if (st) { printf("StaticInit=%d\n", st); return -110 - st; }

    st = VL53L0X_PerformRefSpadManagement(&vl53_dev, &spad_count, &is_aperture);
    if (st) { printf("RefSpad=%d\n", st); return -120 - st; }

    st = VL53L0X_PerformRefCalibration(&vl53_dev, &vhv, &phase);
    if (st) { printf("RefCal=%d\n", st); return -130 - st; }

    st = VL53L0X_SetDeviceMode(&vl53_dev, VL53L0X_DEVICEMODE_SINGLE_RANGING);
    if (st) { printf("SetMode=%d\n", st); return -140 - st; }

    st = VL53L0X_SetMeasurementTimingBudgetMicroSeconds(&vl53_dev, 33000);
    if (st) { printf("TimingBudget=%d\n", st); return -150 - st; }

    return 0;
}

int vl53_read_mm(uint16_t *mm) {
    if (!mm) return -1;

    VL53L0X_RangingMeasurementData_t m = {0};
    VL53L0X_Error st = VL53L0X_PerformSingleRangingMeasurement(&vl53_dev, &m);
    if (st) {
        return -200 - st;
    }

    if (m.RangeStatus != 0) {
        *mm = 0;
        return (int)m.RangeStatus;
    }

    *mm = (uint16_t)m.RangeMilliMeter;
    return 0;
}
