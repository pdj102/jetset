#ifndef ZXN_HW_H
#define ZXN_HW_H

#include <stdint.h>

enum zxn_hw_cpu_speed
{
    ZXN_HW_CPU_3MHZ = 0,
    ZXN_HW_CPU_7MHZ = 1,
    ZXN_HW_CPU_14MHZ = 2,
    ZXN_HW_CPU_28MHZ = 3
};

void zxn_hw_init(void);
void zxn_video_init(void);
void zxn_audio_init(void);
void zxn_hw_disable_contention(void);
void zxn_hw_set_cpu_speed(enum zxn_hw_cpu_speed speed);

#endif