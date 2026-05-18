#ifndef _CODAGE_RLE_
#define _CODAGE_RLE_

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include "htables.h"

typedef struct {
    int classe;
    int indice;
} Coeff_Huff;

typedef struct {
    int chemin;
    int profondeur;
} Chemin_Huff;

void write_bits(FILE *f, int val, int nbr_bits);
void flush_bits(FILE *f);
Coeff_Huff magnitude(int val);
Chemin_Huff codage_Huff(int classe, const uint8_t counts[], const uint8_t symbols[]);
void chaine_Huff_coeff(FILE *f, int16_t coeff, int cpt_zeros, bool is_DC, bool is_Y, bool is_Cb, int predicateur);
void chaine_Huff_vect(FILE *f, int16_t *coeffs, bool is_Y, bool is_Cb, int predicateur);

#endif /* _CODAGE_RLE_ */