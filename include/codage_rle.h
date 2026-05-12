// #ifndef CODAGE_RLE
// #define CODAGE_RLE

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

void flush_bits(FILE *f);
void chaine_Huff_coeff(FILE *f, int16_t coeff, int cpt_zeros, bool is_DC, bool is_Y, bool is_Cb, int predicateur);
void chaine_Huff_vect(FILE *f, int16_t *coeffs, bool is_Y, bool is_Cb, int predicateur);

/* mode progressif */
// void chaine_Huff_vect_progressif(FILE *f, int16_t *coeffs, bool is_Y, bool is_Cb, int predicateur, int debut, int nbr_coeffs);

// #endif