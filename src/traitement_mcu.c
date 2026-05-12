#include "traitement_mcu.h"

/*
    entrée  :   un vecteur "vect_img" de taille "(h1 * 8) * (v1 * 8)"
                tel que :
                                |    val    si pgm |
                vect_img[i] =   |                  |
                                | [R, G, B] si ppm |


    sortie  :   un vecteur "mcu_flow" de taille "h1 * v1 + h2 * v2 + h3 * v3"
                tel que "mcu_flow[k] = b"
                où "b" bloc contient :
                    1. nature du bloc 8 * 8 pixel (Y ou Cb ou Cr)
                    2. un vecteur "data" de taille 64 (après dct, zig-zag, et quantification)
*/

/*
    les parametres de "traitement_mcu" : 

    -> constante 1 : (nbr_pixels)

        int nbr_pixels = s.h[0] * s.v[0] * 64;

    -> constante 2 : (nbr_bloc_mcu)

        int blocs_Y  = s.h[0] * s.v[0];
        int blocs_Cb = s.h[1] * s.v[1];
        int blocs_Cr = s.h[2] * s.v[2];

        int nbr_bloc_mcu;

        if (colors == 0) { nbr_bloc_mcu = blocs_Y;                       } 
        else             { nbr_bloc_mcu = blocs_Y + blocs_Cb + blocs_Cr; }
    
    -> constante 3 : (cos_table)

        double cos_table[64];
        cos_init(cos_table);

    -> constante 4 : (colors)

        if (ppm)     { colors = 0; }
        else         { colors = 1; }
*/

void traitement_mcu(rgb_mcu *vect_img, bloc *mcu_flow, sampling_factors s, int nbr_bloc_mcu, double *cos_table, int colors, ycbcr_mcu *tmp_1, bloc *tmp_2) {

    /* etape_1 : conversion RGB en YCbCr */
    rgb_ycbcr(vect_img, tmp_1, s, colors);

    /* etape_2 : mcu_compression et downsampling */
    mcu_flow_writing(tmp_1, tmp_2, s, colors);

    /* etape_3 : dct, zig-zag et quantification */
    dct_application(tmp_2, mcu_flow, nbr_bloc_mcu, cos_table);
    zz_application(mcu_flow, tmp_2, nbr_bloc_mcu);
    quantification_application(tmp_2, mcu_flow, nbr_bloc_mcu);
}