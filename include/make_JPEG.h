#include <stdio.h>
#include <stdint.h>
#include "qtables.h"
#include "htables.h"
#include <stdbool.h>
#include "mcu_compression.h"
#include "codage_rle.h"

void make_JPEG(FILE *f, uint16_t hauteur, uint16_t largeur, uint8_t nb_couleurs, sampling_factors s, int nb_blocs, bloc *blocs);