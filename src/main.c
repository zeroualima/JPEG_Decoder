#include <stdlib.h>
#include "htables.h"
#include "qtables.h"

#include "parser.h"
#include "codage_rle.h"


int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    //

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
    } 

    FILE *f_lire = fopen(argv[1], "rb");
    if (!f_lire) {
        perror("fopen");
    }

    // FILE *f_ecrire = fopen(argv[2], "wb");

    // Test Resize
    Image *image = read_file(f_lire, 1, 1, 5, 5);
    free(image->data);
    free(image);

    // Test magnitude
    // printf("%d\n", magnitude(24));

    // Test RLE
    // int *coeffs = malloc(64 * sizeof(int));
    // for (int i = 0; i < 64; i++) {
    //     coeffs[i] = i + 1;
    //     printf("%d ", coeffs[i]);
    // }
    // printf("\n");
    // rle(coeffs, 0);


    // Test Chemin Huff
    // Chemin_Huff new = codage_Huff(4, htables_nb_symb_per_lengths[0][0], htables_symbols_DC_Y);
    // printf("%b\n", new.chemin);
    // printf("%d\n", new.profondeur);

    // chaine_Huff_coeff
    // chaine_Huff_coeff(-66, 0, false, true, false, -1);
    // printf("\n");

    // int table[64] = {
    //     24,   0,  -1,  -66,   0,  -23,   0,  -16,
    //     0, -50,  31,    0,  -14,   0,   0,    0,
    //     26,   0, -19,    0,   -7,  -8,   0,   16,
    //     0,  13,   0,   -2,    0,  -5,   0,    1,
    //     0,   2,   0,   12,    0,   0,   0,   11,
    //     0,  13,   0,    0,    0,   0,   3,    0,
    //     -5,   0,  -7,    0,    0,   0,   0,  -16,
    //     0,  -2,   0,    8,    0,   0,  -6,    0
    // };
    // chaine_Huff_vect(&(table[0]), true, false, 0);


    //
    return EXIT_SUCCESS;
}
