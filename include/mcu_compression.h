#ifndef _MCU_COMPRESSION_H_
#define _MCU_COMPRESSION_H_

#include "rgb_ycbcr.h"

/* structures */
typedef struct {    

    int id;         /* identifiant du mcu */

    int nbr_blocs;  /* nbr de blocs 8*8 utils */

    int16_t Y[4][64] ;  /* les blocs Y  */
    int16_t Cb[4][64];  /* les blocs Cb */
    int16_t Cr[4][64];  /* les blocs Cr */

} mcu;

typedef enum {
    Y,
    Cb,
    Cr
} bloc_type;

typedef struct {
    int16_t data[64];
    bloc_type type;
} bloc;

/* prototypes */
void test_sampling_factors(sampling_factors s);
int nbr_mcu(int width, int height, sampling_factors s);
void bloc_Y(uint8_t *src, int src_width, int base_x, int base_y, int dx, int dy, int16_t bloc[64]);
void bloc_Cb_Cr(uint8_t *src, int src_width, int base_x, int base_y, int dx, int dy, int h_factor, int v_factor, int16_t bloc[64]);
void mcu_compresssion(ycbcr *data_ycbcr, mcu *mcus, sampling_factors s);
void blocs_writing(mcu *mcus, bloc *mcu_flow, int nbr_mcu, sampling_factors s, int colors);

#endif /* _MCU_COMPRESSION_H_ */