#ifndef _RGB_YCBCR_H_
#define _RGB_YCBCR_H_

#include "parser_resize.h"

/* structures */
typedef struct {
    uint8_t *Y;
    uint8_t *Cb;
    uint8_t *Cr;
} ycbcr_mcu;

/* prototypes */
void rgb_ycbcr(rgb_mcu *vect_img, ycbcr_mcu *new_vect_img, sampling_factors s, int colors);

#endif /* _RGB_YCBCR_H_ */