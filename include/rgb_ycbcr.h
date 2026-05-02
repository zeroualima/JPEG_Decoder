#ifndef _RGB_YCBCR_H_
#define _RGB_YCBCR_H_

#include "parser_resize.h"

/* structures */
typedef struct {
    int width;
    int height;

    int colors; /* 0 for Gray, 1 for RGB */

    uint8_t *Y;
    uint8_t *Cb;
    uint8_t *Cr;
} ycbcr;

/* prototypes */
void RGB_2_YCbCr(Image *img, ycbcr *new_img);

#endif /* _RGB_YCBCR_H_ */