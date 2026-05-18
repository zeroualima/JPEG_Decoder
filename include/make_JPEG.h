#ifndef _MAKE_JPEG_
#define _MAKE_JPEG_

#include <stdio.h>
#include <stdint.h>
#include "qtables.h"
#include "htables.h"
#include <stdbool.h>
#include "mcu_compression.h"
#include "codage_rle.h" 

extern uint8_t soi[2];
extern uint8_t eoi[2];
extern uint8_t app0[18]; // taille fixe de app0 pour faire passer les liens necessitant une taille fixe a la compilation

extern int predY;
extern int predCb;
extern int predCr;

void write_DQT(FILE *f, uint8_t iQ, const uint8_t table_quantification[64]);
void write_SOF(FILE *f, uint8_t marqueur, uint16_t hauteur, uint16_t largeur, uint8_t nb_couleurs, uint8_t fH_Y, uint8_t fV_Y, uint8_t fH_Cb, uint8_t fV_Cb,  uint8_t fH_Cr, uint8_t fV_Cr);
void write_DHT(FILE *f, uint8_t is_AC, uint8_t iH, const uint8_t nb_symb_per_lengths[16], const uint8_t *symboles);
void write_SOS(FILE *f, uint8_t nb_couleurs, uint8_t iH_DC_Y, uint8_t iH_AC_Y, uint8_t iH_DC_C, uint8_t iH_AC_C, uint8_t SS, uint8_t SE, uint8_t AhAl);
void add_JPEG_entete(FILE *f, uint16_t hauteur, uint16_t largeur, uint8_t nb_couleurs, sampling_factors s);
void add_JPEG_total_bitstream(FILE *f, int nb_blocs, bloc *blocs);
void add_JPEG_end(FILE *f);

#endif /* _MAKE_JPEG_ */