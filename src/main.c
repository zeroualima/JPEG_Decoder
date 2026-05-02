#include <stdlib.h>
#include "htables.h"
#include "qtables.h"
#include <stdio.h>
#include <stdbool.h>

#include "parser_resize.h"
#include "rgb_ycbcr.h"
#include "mcu_compression.h"
#include "make_JPEG.h"
#include "dct.h"
#include "zz.h"
#include "quantification.h"


int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    //
    // if (argc != 3) {
    //     fprintf(stderr, "Usage: %s <file>\n", argv[0]);
    // } 
    // FILE *f_lire = fopen(argv[1], "rb");
    // if (!f_lire) {
    //     perror("fopen lecture");
    //     exit(1);
    // }
    // FILE *f_ecrire = fopen(argv[2], "wb");
    // if (!f_ecrire) {
    //     perror("fopen ecriture");
    //     fclose(f_lire);  // fermer le premier avant de quitter
    //     exit(1);
    // }
    //

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

/* ===================== LECTURE ===================== */
    printf("lecture image\n");
    Image *image = malloc(sizeof(Image));
    read_file(f_lire, image);
    printf("Image %dx%d colors=%d\n", image->width, image->height, image->colors);


/* ===================== SAMPLING ===================== */
    sampling_factors s;
    if (image->colors == 1) {
        s.h[0] = 1; s.v[0] = 1;
        s.h[1] = 1; s.v[1] = 1;
        s.h[2] = 1; s.v[2] = 1;
    } else {
        s.h[0] = 2; s.v[0] = 2;
        s.h[1] = 1; s.v[1] = 1;
        s.h[2] = 1; s.v[2] = 1;
    }
    test_sampling_factors(s);
    Image *img = malloc(sizeof(Image));
    resize(image, img, s);

/* ===================== YCBCR ===================== */
    int nb_pixels = img->width * img->height;

    ycbcr img_yc;
    img_yc.width = img->width;
    img_yc.height = img->height;
    img_yc.colors = img->colors;

    img_yc.Y  = malloc(nb_pixels);
    img_yc.Cb = malloc(nb_pixels);
    img_yc.Cr = malloc(nb_pixels);

    RGB_2_YCbCr(img, &img_yc);

/* ===================== MCU ===================== */
    printf("MCU compression\n");

    int nb_mcu = nbr_mcu(img_yc.width, img_yc.height, s);
    mcu *mcus = malloc(nb_mcu * sizeof(mcu));
    mcu_compresssion(&img_yc, mcus, s);

/* ===================== FLUX MCU ===================== */
    int blocs_Y  = s.h[0] * s.v[0];
    int blocs_Cb = s.h[1] * s.v[1];
    int blocs_Cr = s.h[2] * s.v[2];

    int blocs_par_mcu;
    if (img_yc.colors == 0) {
        blocs_par_mcu = blocs_Y;
    } else {
        blocs_par_mcu = blocs_Y + blocs_Cb + blocs_Cr;
    }

    printf("nbr_mcu/nbr_bloc_mcu = %d/%d\n", nb_mcu, blocs_par_mcu);

    int nb_blocs = nb_mcu * blocs_par_mcu;

    printf("DEBUG : nb_mcu=%d blocs_par_mcu=%d nb_blocs=%d sizeof(bloc)=%zu\n", nb_mcu, blocs_par_mcu, nb_blocs, sizeof(bloc));
        
    bloc *mcu_flow = malloc(nb_blocs * sizeof(bloc));

    blocs_writing(mcus, mcu_flow, nb_mcu, s, img_yc.colors);

    free(mcus);
    free(img_yc.Y);
    free(img_yc.Cb);
    free(img_yc.Cr);

/* ===================== DCT ===================== */
    printf("DCT\n");

    double cos_table[64];
    cos_init(cos_table);

    bloc *dct_flow = malloc(nb_blocs * sizeof(bloc));
    dct_application(mcu_flow, dct_flow, nb_blocs, cos_table);
    free(mcu_flow);

/* ===================== ZIGZAG ===================== */
    printf("Zig-Zag\n");
    bloc *zz_flow = malloc(nb_blocs * sizeof(bloc));
    zz_application(dct_flow, zz_flow, nb_blocs);
    free(dct_flow);

/* ===================== QUANTIFICATION ===================== */
    printf("Quantification\n");
    bloc *q_flow = malloc(nb_blocs * sizeof(bloc));
    for (int b = 0; b < nb_blocs; b++) {

        quantification_bloc(
            zz_flow[b].data,
            q_flow[b].data,
            zz_flow[b].type
        );

        q_flow[b].type = zz_flow[b].type;
    }
    free(zz_flow);

/* ===================== ECRITURE (EN-TETE + HUFFMANN BITSTREAM) ===================== */
    uint16_t hauteur = image->height;
    uint16_t largeur = image->width;
    uint8_t nb_couleurs = img->colors;

    printf("%d %d\n", hauteur, largeur);
    
    make_JPEG(f_ecrire, hauteur, largeur, nb_couleurs, s, nb_blocs, q_flow);

    free(q_flow);
    free(image->data);
    free(image);
    free(img->data);
    free(img);

    //
    return EXIT_SUCCESS;
}
