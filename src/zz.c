#include "zz.h"

void zz(uint8_t *block) {
	(void)block;
}

/* zig-zag sur un bloc du flux DCT (bloc de 8*8 sous forme de vecteur) */
void zz_bloc(int16_t *input, int16_t *output) {
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            output[j * 8 + i] = input[zigzag[j * 8 + i]];
        }
    }
}

/* aplication de zig-zag sur tout le flux */
void zz_application(bloc *dct_flow, bloc *zz_flow, int nbr_blocs) {
    for (int b = 0; b < nbr_blocs; b++) {
        zz_bloc(dct_flow[b].data, zz_flow[b].data);
        zz_flow[b].type = dct_flow[b].type;
    }
}