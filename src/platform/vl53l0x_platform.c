#include "vl53l0x_platform.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <string.h> 
#define I2C_PORT i2c0
#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 5


static void platform_delay_ms(uint32_t ms) {
    sleep_ms(ms);
}

VL53L0X_Error VL53L0X_LockSequenceAccess(VL53L0X_DEV Dev) {
    return VL53L0X_ERROR_NONE; 
}

VL53L0X_Error VL53L0X_UnlockSequenceAccess(VL53L0X_DEV Dev) {
    return VL53L0X_ERROR_NONE; 
}

VL53L0X_Error VL53L0X_WriteMulti(VL53L0X_DEV Dev, uint8_t index, uint8_t *pdata, uint32_t count) {
    uint8_t buf[count + 1];
    buf[0] = index;
    memcpy(&buf[1], pdata, count);
    int ret = i2c_write_blocking(I2C_PORT, Dev->I2cDevAddr, buf, count + 1, false);
    return (ret < 0) ? VL53L0X_ERROR_CONTROL_INTERFACE : VL53L0X_ERROR_NONE;
}

VL53L0X_Error VL53L0X_ReadMulti(VL53L0X_DEV Dev, uint8_t index, uint8_t *pdata, uint32_t count) {
    int ret = i2c_write_blocking(I2C_PORT, Dev->I2cDevAddr, &index, 1, true);
    if (ret < 0) return VL53L0X_ERROR_CONTROL_INTERFACE;
    ret = i2c_read_blocking(I2C_PORT, Dev->I2cDevAddr, pdata, count, false);
    return (ret < 0) ? VL53L0X_ERROR_CONTROL_INTERFACE : VL53L0X_ERROR_NONE;
}

VL53L0X_Error VL53L0X_WrByte(VL53L0X_DEV Dev, uint8_t index, uint8_t data) {
    return VL53L0X_WriteMulti(Dev, index, &data, 1);
}

VL53L0X_Error VL53L0X_WrWord(VL53L0X_DEV Dev, uint8_t index, uint16_t data) {
    uint8_t buf[2];
    buf[0] = data >> 8;
    buf[1] = data & 0xFF;
    return VL53L0X_WriteMulti(Dev, index, buf, 2);
}

VL53L0X_Error VL53L0X_WrDWord(VL53L0X_DEV Dev, uint8_t index, uint32_t data) {
    uint8_t buf[4];
    buf[0] = (data >> 24) & 0xFF;
    buf[1] = (data >> 16) & 0xFF;
    buf[2] = (data >> 8) & 0xFF;
    buf[3] = data & 0xFF;
    return VL53L0X_WriteMulti(Dev, index, buf, 4);
}

VL53L0X_Error VL53L0X_RdByte(VL53L0X_DEV Dev, uint8_t index, uint8_t *data) {
    return VL53L0X_ReadMulti(Dev, index, data, 1);
}

VL53L0X_Error VL53L0X_RdWord(VL53L0X_DEV Dev, uint8_t index, uint16_t *data) {
    uint8_t buf[2];
    VL53L0X_Error status = VL53L0X_ReadMulti(Dev, index, buf, 2);
    *data = ((uint16_t)buf[0] << 8) | buf[1];
    return status;
}

VL53L0X_Error VL53L0X_RdDWord(VL53L0X_DEV Dev, uint8_t index, uint32_t *data) {
    uint8_t buf[4];
    VL53L0X_Error status = VL53L0X_ReadMulti(Dev, index, buf, 4);
    *data = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) | ((uint32_t)buf[2] << 8) | buf[3];
    return status;
}

VL53L0X_Error VL53L0X_UpdateByte(VL53L0X_DEV Dev, uint8_t index, uint8_t AndData, uint8_t OrData) {
    uint8_t data;
    VL53L0X_Error status = VL53L0X_RdByte(Dev, index, &data);
    if (status != VL53L0X_ERROR_NONE) return status;
    data = (data & AndData) | OrData;
    return VL53L0X_WrByte(Dev, index, data);
}

VL53L0X_Error VL53L0X_PollingDelay(VL53L0X_DEV Dev) {
    platform_delay_ms(5);
    return VL53L0X_ERROR_NONE;
}

static void vl53_i2c_bus_init(void) {
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
}

