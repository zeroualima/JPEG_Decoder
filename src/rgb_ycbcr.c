#include "rgb_ycbcr.h"

/* pour la version de lecture MCU par MCU */
void rgb_ycbcr(rgb_mcu *vect_img, ycbcr_mcu *new_vect_img, sampling_factors s, int colors) {
    int mcu_width  = s.h[0] * 8;
    int mcu_height = s.v[0] * 8;

    int taille = mcu_width * mcu_height;
    
    /* pour pgm */
    if (colors == 0) {
        for (int i = 0; i < taille; i++) {
            new_vect_img->Y[i]  = vect_img->data[i][0];
            new_vect_img->Cb[i] = 128;
            new_vect_img->Cr[i] = 128;
        }
        return;
    }

    /* pour ppm */
    for (int y = 0; y < mcu_height; y++) {
        for (int x = 0; x < mcu_width; x++) {

            int idx = y * mcu_width + x;

            uint8_t r = vect_img->data[idx][0];
            uint8_t g = vect_img->data[idx][1];
            uint8_t b = vect_img->data[idx][2];

            new_vect_img->Y[idx]  =  0.2990 * r + 0.5870 * g + 0.1140 * b;
            new_vect_img->Cb[idx] = -0.1687 * r - 0.3313 * g + 0.5000 * b + 128;
            new_vect_img->Cr[idx] =  0.5000 * r - 0.4187 * g - 0.0813 * b + 128;
        }
    }

    return;
}