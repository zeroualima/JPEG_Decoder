#include "mcu_compression.h"

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

int nbr_mcu(int width, int height, sampling_factors s) {
    
    int mcu_width  = s.h[0] * 8;
    int mcu_height = s.v[0] * 8;

    int nbr_mcu_x = width  / mcu_width;
    int nbr_mcu_y = height / mcu_height;

    int nbr_mcu = nbr_mcu_x * nbr_mcu_y;

    return nbr_mcu; 
}

/* remplisage d'un bloc 8x8 de Y */
void bloc_Y(uint8_t *src, int src_width, int base_x, int base_y, int dx, int dy, int16_t bloc[64]) {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int ix  = base_x + dx*8 + x;
            int iy  = base_y + dy*8 + y;
            bloc[y*8 + x] = (int16_t)src[iy * src_width + ix];
        }
    }
}

/* remplisage d'un bloc 8x8 de CbCr (avec sous-échantillonnage) */
void bloc_Cb_Cr(uint8_t *src, int src_width, int base_x, int base_y, int dx, int dy, int h_factor, int v_factor, int16_t bloc[64]) {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int base_px = base_x + dx * 8 * h_factor + x * h_factor;
            int base_py = base_y + dy * 8 * v_factor + y * v_factor;
            int sum = 0;
            for (int py = 0; py < v_factor; py++) {
                for (int px = 0; px < h_factor; px++) {
                    sum += src[(base_py + py) * src_width + (base_px + px)];
                }
            }
            bloc[y*8 + x] = (int16_t)(sum / (h_factor * v_factor));
        }
    }
}


void mcu_compresssion(ycbcr *data_ycbcr, mcu *mcus, sampling_factors s) {
    
    /* parametre de sampling_factors */
    int h1 = s.h[0], v1 = s.v[0];
    int h2 = s.h[1], v2 = s.v[1];
    int h3 = s.h[2], v3 = s.v[2];
    
    /* dimension de MCU */
    int mcu_width  = h1 * 8;
    int mcu_height = v1 * 8;

    /* nbr de bloc apres decomposition de ycbcr en bloc MCU */
    int nbr_bloc_x = data_ycbcr->width  / mcu_width;
    int nbr_bloc_y = data_ycbcr->height / mcu_height;

    int h_cb = h1 / h2;
    int v_cb = v1 / v2;
    int h_cr = h1 / h3;
    int v_cr = v1 / v3;

    // /* dimension de Cb et Cr apres sampling */
    // int cb_width  = data_ycbcr->width  / h_cb;
    // int cb_height = data_ycbcr->height / v_cb;
    // int cr_width  = data_ycbcr->width  / h_cr;
    // int cr_height = data_ycbcr->height / v_cr;

    /* id de MCU (idx de mcu dans la liste mcus) */
    int id = 0;

    /* remplisage de la liste mcus */
    for (int y_mcu = 0; y_mcu < nbr_bloc_y; y_mcu++) {
        for (int x_mcu = 0; x_mcu < nbr_bloc_x; x_mcu++) {

            mcu *cur = &mcus[id];
            cur->id  = id;

            /* les coordonnes du MCU courant */
            int base_x = x_mcu * mcu_width;
            int base_y = y_mcu * mcu_height;

            /* Y */
            for (int dy = 0; dy < v1; dy++) {
                for (int dx = 0; dx < h1; dx++) {
                    bloc_Y(data_ycbcr->Y, data_ycbcr->width, base_x, base_y, dx, dy, cur->Y[dy*h1 + dx]);
                }
            }

            if (data_ycbcr->colors == 0) { 
                id++; 
                continue; 
            }

            /* Cb */
            for (int dy = 0; dy < v2; dy++) {
                for (int dx = 0; dx < h2; dx++) {
                    bloc_Cb_Cr(data_ycbcr->Cb, data_ycbcr->width, base_x, base_y, dx, dy, h_cb, v_cb, cur->Cb[dy*h2 + dx]);
                }
            }
            
            /* Cr */
            for (int dy = 0; dy < v3; dy++) {
                for (int dx = 0; dx < h3; dx++) {
                    bloc_Cb_Cr(data_ycbcr->Cr, data_ycbcr->width, base_x, base_y, dx, dy, h_cr, v_cr, cur->Cr[dy*h3 + dx]);
                }
            }

            id++;
        }
    }
}

/* 
    apres construction de la liste "mcus" des MCU,
    on la parcourt pour ecrire le flux 
    (n'oublier pas "colors = data_ycbcr->colors;" dans main)
*/
void blocs_writing(mcu *mcus, bloc *mcu_flow, int nbr_mcu, sampling_factors s, int colors) {
    
    int h1 = s.h[0], v1 = s.v[0];
    int h2 = s.h[1], v2 = s.v[1];
    int h3 = s.h[2], v3 = s.v[2];

    int idx = 0;

    for (int i = 0; i < nbr_mcu; i++) {

        /* les blocs Y */
        for (int dy = 0; dy < v1; dy++) {
            for (int dx = 0; dx < h1; dx++) {
                memcpy(mcu_flow[idx].data, mcus[i].Y[dy * h1 + dx], 64 * sizeof(int16_t));
                mcu_flow[idx].type = Y;
                idx++;
            }
        }

        if (colors == 1) {
            
            /* les blocs Cb */
            for (int dy = 0; dy < v2; dy++) {
                for (int dx = 0; dx < h2; dx++) {    
                    memcpy(mcu_flow[idx].data, mcus[i].Cb[dy * h2 + dx], 64 * sizeof(int16_t));
                    mcu_flow[idx].type = Cb;
                    idx++;
                }
            }

            /* les blocs Cr */
            for (int dy = 0; dy < v3; dy++) {
                for (int dx = 0; dx < h3; dx++) {
                    memcpy(mcu_flow[idx].data, mcus[i].Cr[dy * h3 + dx], 64 * sizeof(int16_t));
                    mcu_flow[idx].type = Cr;
                    idx++;
                }
            }
        }
    }
}