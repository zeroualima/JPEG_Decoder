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

void chaine_Huff_vect(FILE *f, int *coeffs, bool is_Y, bool is_Cb, int predicateur);

// #endif