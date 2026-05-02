#include "rgb_ycbcr.h"

void RGB_2_YCbCr(Image *img, ycbcr *new_img) {
    int width  = img->width ;
    int height = img->height;
    
    int taille = width * height;

    new_img->width = width ;
    new_img->height = height;

    /* pour pgm */
    if (img->colors == 1) {
        for (int i = 0; i < taille; i++) {
            new_img->Y[i]  = img->data[i];
            new_img->Cb[i] = 128;
            new_img->Cr[i] = 128;
            new_img->colors = 0;
        }
        return;
    }

    /* pour ppm */
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {

            uint8_t r = R(img, x, y);
            uint8_t g = G(img, x, y);
            uint8_t b = B(img, x, y);

            int idx = y * width + x;

            new_img->Y[idx]  =  0.2990 * r + 0.5870 * g + 0.1140 * b;
            new_img->Cb[idx] = -0.1687 * r - 0.3313 * g + 0.5000 * b + 128;
            new_img->Cr[idx] =  0.5000 * r - 0.4187 * g - 0.0813 * b + 128;

            new_img->colors = 1;
        }
    }

    return;
}

