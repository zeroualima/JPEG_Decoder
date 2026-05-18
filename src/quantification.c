#include "quantification.h"

/* quantification sur un bloc du flux DCT (bloc de 8*8 sous forme de vecteur) */
void quantification_bloc(int16_t *input, int16_t *output, bloc_type bloc) {
    
    if (bloc == Y) {
        /* si le bloc 8*8 est un Y */
        for (int idx = 0; idx < 64; idx++) {
            /* 
                cas possible :
                    -> output[idx] = (uint16_t)round(       input[idx] /        qtable[idx]);
                    -> output[idx] = (uint16_t)round((float)input[idx] / (float)qtable[idx]);
                ou bien en utilise "floor" au lieu de "round"
            */
            output[idx] = (int16_t)round((float)input[idx] / (float)quantification_table_Y[idx]);
        }
    } else {
        /* si le bloc 8*8 est un Cb ou Cr */
        for (int idx = 0; idx < 64; idx++) {
            output[idx] = (int16_t)round((float)input[idx] / (float)quantification_table_CbCr[idx]);

        }
    }
}

/* aplication de quantification sur tout le flux */
void quantification_application(bloc *zz_flow, bloc *q_flow, int nbr_blocs) {
    for (int b = 0; b < nbr_blocs; b++) {
        quantification_bloc(zz_flow[b].data, q_flow[b].data, zz_flow[b].type);
        q_flow[b].type = zz_flow[b].type;
    }
}