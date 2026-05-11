#include <stdio.h>
#include <stdint.h>
#include "qtables.h"
#include "htables.h"
#include <stdbool.h>
#include "mcu_compression.h"
#include "codage_rle.h"

void write_DHT(FILE *f, uint8_t is_AC, uint8_t iH, const uint8_t nb_symb_per_lengths[16], const uint8_t *symboles);
void write_SOS_progressif(FILE *f, uint8_t nb_comp, uint8_t scan, uint8_t nb_couleurs, uint8_t SS, uint8_t SE, uint8_t AhAl);
void add_JPEG_entete(FILE *f, uint16_t hauteur, uint16_t largeur, uint8_t nb_couleurs, sampling_factors s);
void add_JPEG_entete_progressif(FILE *f, uint16_t hauteur, uint16_t largeur, uint8_t nb_couleurs, sampling_factors s);
void add_JPEG_total_bitstream(FILE *f, int nb_blocs, bloc *blocs);
void add_JPEG_total_bitstream_progressif(FILE *f, int nb_blocs, bloc *blocs, int debut, int fin, int nbr_scan);
// void add_JPEG_bitstream(FILE *f, bloc bloc);
void add_JPEG_end(FILE *f);


void write_SOS(FILE *f, uint8_t nb_couleurs, uint8_t iH_DC_Y, uint8_t iH_AC_Y, uint8_t iH_DC_C, uint8_t iH_AC_C, uint8_t SS, uint8_t SE, uint8_t AhAl);