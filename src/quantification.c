#include "quantification.h"

/* quantification sur un bloc du flux DCT (bloc de 8*8 sous forme de vecteur) */
void quantification_bloc(int16_t input[64], int16_t output[64], bloc_type bloc) {
    
    if (bloc == 0) {
        /* si le bloc 8*8 est un Y */
        for (int i = 0; i < 64; i++) {
            /* 
                cas possible :
                    -> output[i] = (uint16_t)round(       input[i] /        qtable[i]);
                    -> output[i] = (uint16_t)round((float)input[i] / (float)qtable[i]);
                ou bien en utilise "floor" au lieu de "round"
            */
            output[i] = (int16_t)round((float)input[i] / (float)quantification_table_Y[i]);
        }
    } else {
        /* si le bloc 8*8 est un Cb ou Cr */
        for (int i = 0; i < 64; i++) {
            output[i] = (int16_t)round((float)input[i] / (float)quantification_table_CbCr[i]);
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