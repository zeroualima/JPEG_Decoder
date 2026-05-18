#ifndef _MCU_COMPRESSION_H_
#define _MCU_COMPRESSION_H_

#include "rgb_ycbcr.h"

typedef enum {
    Y,
    Cb,
    Cr
} bloc_type;

typedef struct {
    int16_t data[64];
    bloc_type type; 
} bloc;

void test_sampling_factors(sampling_factors s);
// int nbr_mcu(int width, int height, sampling_factors s);
void bloc_Y(uint8_t *Y, int mcu_width, int origine_x, int origine_y, int16_t bloc[64]);
void bloc_Cb_Cr(uint8_t *Cb_Cr, int mcu_width, int origine_x, int origine_y, int h_factor, int v_factor, int16_t bloc[64]);
void mcu_flow_writing(ycbcr_mcu *data_ycbcr, bloc *mcu_flow, sampling_factors s, int colors);

#endif /* _MCU_COMPRESSION_H_ */