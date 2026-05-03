#include <stdio.h>

#include "parser_resize.h"
#include "traitement_mcu.h"
#include "make_JPEG.h"

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        exit(1);
    }

    FILE *f_lire = fopen(argv[1], "rb");
    if (!f_lire) {
        perror("fopen lecture");
        exit(1);
    }

    // Find the filename after the last '/'
    char *slash = strrchr(argv[1], '/');
    char *filename = slash ? slash + 1 : argv[1];

    // Find the extension (last '.')
    char *dot = strrchr(filename, '.');
    if (!dot) {
        fprintf(stderr, "Erreur : pas d'extension trouvée dans '%s'\n", argv[1]);
        fclose(f_lire);
        exit(1);
    }
    int nb_colors = (strcmp(dot, ".pgm") == 0) ? 1 : 3; 
    // Base name length (without extension)
    int base_len = dot - filename;

    // Build "out/<basename>.jpg\0"  →  4 + base_len + 4 + 1
    int output_len = 4 + base_len + 4 + 1;
    char *output_path = malloc(output_len);
    if (!output_path) {
        perror("malloc");
        fclose(f_lire);
        exit(1);
    }
    snprintf(output_path, output_len, "out/%.*s.jpg", base_len, filename);

    FILE *f_ecrire = fopen(output_path, "wb");  // ← output_path, not argv[1]
    if (!f_ecrire) {
        perror("fopen ecriture");
        fclose(f_lire);
        free(output_path);
        exit(1);
    }
    free(output_path);



/* ===================== SAMPLING ===================== */
    sampling_factors s;
    if (nb_colors == 1) {
        s.h[0] = 1; s.v[0] = 1;
        s.h[1] = 1; s.v[1] = 1;
        s.h[2] = 1; s.v[2] = 1;
    } else {
        s.h[0] = 2; s.v[0] = 2;
        s.h[1] = 1; s.v[1] = 1;
        s.h[2] = 1; s.v[2] = 1;
    }
    test_sampling_factors(s);

/* ===================== LECTURE ===================== */
    printf("lecture image\n");
    Image *image = malloc(sizeof(Image));
    init_image(image, argv[1], s);

/* ===================== ECRITURE ===================== */ 
    int nb_pixels = image->ph * image->pw;
    int colors = (nb_colors == 1) ? 0 : 1;

    int blocs_Y  = s.h[0] * s.v[0];
    int blocs_Cb = s.h[1] * s.v[1];
    int blocs_Cr = s.h[2] * s.v[2];

    int nbr_bloc_mcu = (colors == 0) ? blocs_Y : blocs_Y + blocs_Cb + blocs_Cr;

    rgb_mcu *mcu = malloc(sizeof(rgb_mcu));
    init_mcu(mcu, s);

    double *cos_table = malloc(64 * sizeof(double));
    cos_init(cos_table);

    bloc *blocs = malloc(sizeof(bloc) * nbr_bloc_mcu);

    //
    add_JPEG_entete(f_ecrire, image->h, image->w, nb_colors, s);
    for (int i = 0; i < image->mcu_count; i++) {
        fill_mcu(image, mcu, s, (image->mcus_starting_position)[i]);    
        traitement_mcu(mcu, blocs, s, nb_pixels, nbr_bloc_mcu, cos_table, colors);
        add_JPEG_total_bitstream(f_ecrire, nbr_bloc_mcu, blocs);
    }
    add_JPEG_end(f_ecrire);
    //

    free(mcu->data);
    free(mcu);

    free(image->current_mcu.data);      
    free(image->mcus_starting_position);
    free(image);

    free(cos_table);
    free(blocs);
    
    fclose(f_lire);
    fclose(f_ecrire); 
}