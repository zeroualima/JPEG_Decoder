#include "mcu_compression.h"

/* test de "sampling_factors" */
void test_sampling_factors(sampling_factors s) {
    int sum = 0;

    /* La valeur de chaque facteur h ou v doit être comprise entre 1 et 4 */
    for (unsigned i = 0; i < 3; i++) {
        if (s.h[i] < 1 || s.h[i] > 4 || s.v[i] < 1 || s.v[i] > 4) {
            fprintf(stderr, "ERREUR : unsupported sampling factors\n");
            exit(EXIT_FAILURE);
        }
        sum += s.h[i] * s.v[i];     
    }
    
    /* La somme des produits h*v doit être inférieure ou égale à 10 */   
    if (sum > 10) {
        fprintf(stderr, "ERREUR : unsupported sampling factors\n");
        exit(EXIT_FAILURE);
    }
        
    /* Les facteurs d'échantillonnage des chrominances doivent diviser parfaitement ceux de la luminance */
    for (unsigned i = 1; i < 3; i++) {
        if (s.h[0] % s.h[i] != 0 || s.v[0] % s.v[i] != 0) {
            fprintf(stderr, "ERREUR : unsupported sampling factors\n");
            exit(EXIT_FAILURE);
        }
    }

    return;
}

/* calcule de nombre de MCU totale */
// int nbr_mcu(int width, int height, sampling_factors s) {
    
//     int mcu_width  = s.h[0] * 8;
//     int mcu_height = s.v[0] * 8;

//     int nbr_mcu_x = width  / mcu_width;
//     int nbr_mcu_y = height / mcu_height;

//     int nbr_mcu = nbr_mcu_x * nbr_mcu_y;

//     return nbr_mcu; 
// }

/* remplisage du bloc 8*8 de Y de coordonne (x, y) */
void bloc_Y(uint8_t *Y, int mcu_width, int x, int y, int16_t bloc[64]) {
    for (int dy = 0; dy < 8; dy++) {
        for (int dx = 0; dx < 8; dx++) {
            bloc[dy * 8 + dx] = (int16_t)Y[(y * 8 + dy) * mcu_width + (x * 8 + dx)];
        }
    }
}

/* remplisage du bloc 8*8 de Cb/Cr de coordonne (x, y) (avec sous-échantillonnage) */
void bloc_Cb_Cr(uint8_t *Cb_Cr, int mcu_width, int x, int y, int h_factor, int v_factor, int16_t bloc[64]) {
    for (int dy = 0; dy < 8; dy++) {
        for (int dx = 0; dx < 8; dx++) {
            int origine_x = (x * 8 + dx) * h_factor;
            int origine_y = (y * 8 + dy) * v_factor;
            int sum = 0;
            for (int j = 0; j < v_factor; j++) {
                for (int i = 0; i < h_factor; i++) {
                    int px = origine_x + i;
                    int py = origine_y + j;
                    sum += Cb_Cr[py * mcu_width + px];
                }
            }
            bloc[dy * 8 + dx] = (int16_t)(sum / (h_factor * v_factor));
        }
    }
}

/* 
    cette fct sert à :
        1. remplir les blocs du MCU en cours de traitement 8*8 Y, Cb et Cr (avec downsampling)
        2. ecrire ces blocs en ordre dans "mcu_flow"
*/
void mcu_flow_writing(ycbcr_mcu *data_ycbcr, bloc *mcu_flow, sampling_factors s, int colors) {
    int h1 = s.h[0], v1 = s.v[0];
    int h2 = s.h[1], v2 = s.v[1];
    int h3 = s.h[2], v3 = s.v[2];

    int mcu_width = h1 * 8;

    int h_cb = h1/h2, v_cb = v1/v2;
    int h_cr = h1/h3, v_cr = v1/v3;

    int idx = 0;

    /* Y */
    for (int dy = 0; dy < v1; dy++) {
        for (int dx = 0; dx < h1; dx++) {
            bloc_Y(data_ycbcr->Y, mcu_width, dx, dy, mcu_flow[idx].data);
            mcu_flow[idx].type = Y;
            idx++;
        }
    }

    if (colors == 1) {

        /* Cb */
        for (int dy = 0; dy < v2; dy++) {
            for (int dx = 0; dx < h2; dx++) {
                bloc_Cb_Cr(data_ycbcr->Cb, mcu_width, dx, dy, h_cb, v_cb, mcu_flow[idx].data);
                mcu_flow[idx].type = Cb;
                idx++;
            }
        }

        /* Cr */
        for (int dy = 0; dy < v3; dy++) {
            for (int dx = 0; dx < h3; dx++) {
                bloc_Cb_Cr(data_ycbcr->Cr, mcu_width, dx, dy, h_cr, v_cr, mcu_flow[idx].data);
                mcu_flow[idx].type = Cr;
                idx++;
            }
        }
    }
}