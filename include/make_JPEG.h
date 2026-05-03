#include <stdio.h>
#include <stdint.h>
#include "qtables.h"
#include "htables.h"
#include <stdbool.h>
#include "mcu_compression.h"
#include "codage_rle.h"

void add_JPEG_entete(FILE *f, uint16_t hauteur, uint16_t largeur, uint8_t nb_couleurs, sampling_factors s);
void add_JPEG_total_bitstream(FILE *f, int nb_blocs, bloc *blocs);
// void add_JPEG_bitstream(FILE *f, bloc bloc);
void add_JPEG_end(FILE *f);