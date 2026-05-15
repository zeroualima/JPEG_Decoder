#ifndef _PROGRESSIF_H_
#define _PROGRESSIF_H_

#include "traitement_mcu.h"
#include "codage_rle.h"
#include "make_JPEG.h"

/* structures */


/* prototypes */

/* codage_rle.c */
void chaine_Huff_vect_progressif(FILE *f, int16_t *coeffs, bool is_Y, bool is_Cb, int predicateur, int debut, int nbr_coeffs);

/* make_JPEG.c */
void add_JPEG_entete_progressif(FILE *f, uint16_t hauteur, uint16_t largeur, uint8_t nb_couleurs, sampling_factors s);
void write_SOS_progressif(FILE *f, uint8_t nb_comp, uint8_t scan, uint8_t nb_couleurs, uint8_t SS, uint8_t SE, uint8_t AhAl);
void write_DHT_SOS_progressif(FILE *f, int scan, int nb_colors, int debut, int fin);
void add_JPEG_total_bitstream_progressif(FILE *f, int nb_blocs, bloc *blocs, int debut, int fin, int nbr_scan);
void add_JPEG_end_progressif(FILE *f, int scan, int scan_max);

#endif /* _PROGRESSIF_H_ */