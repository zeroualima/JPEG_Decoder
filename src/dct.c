#include "dct.h"

/* calcule des valeurs cos */
void cos_init(double *cos_table) {
    for (int i = 0; i < 8; i++) {
        for (int x = 0; x < 8; x++) {
            cos_table[i * 8 + x] = cos((2*x + 1) * i * PI / 16.0);
        }
    }
}

/* 
    Phi(i, j) = 2 / n * alpha(i) * alpha(j) * 
                sum_{x=0, n-1} sum_{y=0, n-1} 
                    S(x, y) * 
                    cos((2x + 1) * i * PI / (2n)) * 
                    cos((2y + 1) * j * PI / (2n)) 
*/

/* DCT naive */
void dct_bloc_naive(int16_t *input, int16_t *output, double *cos_table) {
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            double sum = 0.0;
            for (int x = 0; x < 8; x++) {
                for (int y = 0; y < 8; y++) {
                    double pixel = input[y * 8 + x] - 128.0;
                    sum += pixel * cos_table[i * 8 + x] * cos_table[j * 8 + y];
                }
            }
            output[j * 8 + i] = (int16_t)(0.25 * C(i) * C(j) * sum);
        }
    }
}

/* DCT optimisé (séparation 1D) */
void dct_1d(double *input, double *output, double *cos_table) {
    for (int k = 0; k < 8; k++) {
        double sum = 0.0;
        for (int n = 0; n < 8; n++) {
            sum += input[n] * cos_table[k * 8 + n];
        }
        output[k] = sum;
    }
}

void dct_bloc_1d(int16_t *input, int16_t *output, double *cos_table) {
    double tmp[64];

    /* DCT-1D sur les lignes */
    for (int y = 0; y < 8; y++) {
        double row[8];
        for (int x = 0; x < 8; x++) {
            row[x] = input[y * 8 + x] - 128.0;
        }
        dct_1d(row, &tmp[y * 8], cos_table);
    }

    /* DCT-1D sur les colonnes */
    for (int x = 0; x < 8; x++) {
        double col[8];
        for (int y = 0; y < 8; y++) {
            col[y] = tmp[y * 8 + x];
        }

        double col_out[8];
        dct_1d(col, col_out, cos_table);

        for (int y = 0; y < 8; y++) {
            output[y * 8 + x] = (int16_t)(0.25 * C(x) * C(y) * col_out[y]);
        }
    }
}

/* application de DCT sur toutes les blocs */
void dct_application(bloc *mcu_flow, bloc *dct_flow, int nbr_blocs, double *cos_table) {
    for (int b = 0; b < nbr_blocs; b++) {
        /* on utilise la DCT optimisé */
        // dct_bloc_naive(mcu_flow[b].data, dct_flow[b].data, cos_table);
        dct_bloc_1d(mcu_flow[b].data, dct_flow[b].data, cos_table);
        dct_flow[b].type = mcu_flow[b].type;
    }
}