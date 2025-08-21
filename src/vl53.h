#pragma once
#include <stdint.h>

int vl53l0x_platform_init(void);
int vl53_read_mm(uint16_t *mm);
int vl53_run_offset_cal_mm(uint16_t target_mm, int16_t *applied_mm);
int vl53_apply_fixed_offset_mm(int16_t offset_mm_mm);
