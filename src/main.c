#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <syscall.h>

#include "parser_resize.h"
#include "traitement_mcu.h"
#include "make_JPEG.h"

int main(int argc, char **argv) {
    char *infile = NULL;
    char *outfile_arg = NULL;

    char *sample_arg = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            printf("Usage: %s [--outfile=<output.jpg>] [--sample=HxV,HxV,HxV] <input>\n", argv[0]);
            exit(0);
        } else if (strncmp(argv[i], "--outfile=", 10) == 0) {
            outfile_arg = argv[i] + 10;
        } else if (strncmp(argv[i], "--sample=", 9) == 0) {
            sample_arg = argv[i] + 9;
        } else {
            infile = argv[i];
        }
    }

    if (!infile) {
        fprintf(stderr, "Usage: %s [--outfile=<output.jpg>] [--sample=HxV,HxV,HxV] <input>\n", argv[0]);
        exit(1);
    }

   

    char *slash = strrchr(infile, '/');
    char *filename = slash ? slash + 1 : infile;
    char *dot = strrchr(filename, '.');
    if (!dot) {
        fprintf(stderr, "Erreur : pas d'extension trouvée dans '%s'\n", infile);
        exit(1);
    }
    int nb_colors = (strcmp(dot, ".pgm") == 0) ? 1 : 3;

    char *output_path;
    int output_path_allocated = 0;
    if (outfile_arg) {
        output_path = outfile_arg;
    } else {
        int base_len = dot - filename;
        int output_len = 4 + base_len + 4 + 1;
        output_path = malloc(output_len);
        snprintf(output_path, output_len, "out/%.*s.jpg", base_len, filename);
        output_path_allocated = 1;
    }

    FILE *f_ecrire = fopen(output_path, "wb");
    if (!f_ecrire) {
        perror("fopen ecriture");
        if (output_path_allocated) free(output_path);
        exit(1);
    }
    if (output_path_allocated) free(output_path);

    /* SAMPLING */
    sampling_factors s;
    if (sample_arg) {
        int h0, v0, h1, v1, h2, v2;
        if (sscanf(sample_arg, "%dx%d,%dx%d,%dx%d", &h0, &v0, &h1, &v1, &h2, &v2) != 6) {
            fprintf(stderr, "Usage: --sample=HxV,HxV,HxV\n");

            fclose(f_ecrire);
            exit(1);
        }
        s.h[0] = h0; s.v[0] = v0;
        s.h[1] = h1; s.v[1] = v1;
        s.h[2] = h2; s.v[2] = v2;
    } else if (nb_colors == 1) {
        s.h[0] = 1; s.v[0] = 1;
        s.h[1] = 1; s.v[1] = 1;
        s.h[2] = 1; s.v[2] = 1;
    } else {
        s.h[0] = 1; s.v[0] = 1;
        s.h[1] = 1; s.v[1] = 1;
        s.h[2] = 1; s.v[2] = 1;
    }
    test_sampling_factors(s);

    /* LECTURE */
    
    Image *image = malloc(sizeof(Image));
    init_image(image, infile, s);

    /* ECRITURE */
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

    int mcu_pixels = s.h[0] * s.v[0] * 64;
    ycbcr_mcu tmp_ycbcr;
    tmp_ycbcr.Y  = malloc(mcu_pixels);
    tmp_ycbcr.Cb = malloc(mcu_pixels);
    tmp_ycbcr.Cr = malloc(mcu_pixels);
    bloc *tmp_blocs = malloc(nbr_bloc_mcu * sizeof(bloc));

    struct timespec t_start, t_end;
    clock_gettime(CLOCK_MONOTONIC, &t_start);

    int mcu_rows = image->ph / (8 * s.v[0]);
    int mcu_cols = image->pw / (8 * s.h[0]);

    add_JPEG_entete(f_ecrire, image->h, image->w, nb_colors, s); 
    for (int i = 0; i < mcu_rows; i++) {
        for (int j = 0; j < mcu_cols; j++) {
            int pixel_row = i * 8 * s.v[0];
            int pixel_col = j * 8 * s.h[0];
            long mcu_starting_position = image->header_offset + (long)(pixel_row * image->w + pixel_col) * image->colors;
            fill_mcu(image, mcu, s, mcu_starting_position);
            traitement_mcu(mcu, blocs, s, nbr_bloc_mcu, cos_table, colors, &tmp_ycbcr, tmp_blocs);
            add_JPEG_total_bitstream(f_ecrire, nbr_bloc_mcu, blocs);
        }
    }
    add_JPEG_end(f_ecrire);

    // #define SCANS 5
    // int debut[SCANS] = {0, 1, 1, 1, 10};
    // int fin[SCANS] = {0, 9, 63, 63, 63};
    // add_JPEG_entete_progressif(f_ecrire, image->h, image->w, nb_colors, s);
    // for (int scan = 0; scan < SCANS; scan++) {

    //     /* ecriture de DHT et SOS en fct de "scan" */
    //     write_DHT_SOS_progressif(f_ecrire, scan, nb_colors, debut[scan], fin[scan]);

    //     /* traitement de l'MCU + ecriture de bitstream en fct dee "scan" */
    //     for (int i = 0; i < mcu_rows; i++) {
    //         for (int j = 0; j < mcu_cols; j++) {
    //             int pixel_row = i * 8 * s.v[0];
    //             int pixel_col = j * 8 * s.h[0];
    //             long mcu_starting_position = image->header_offset + (long)(pixel_row * image->w + pixel_col) * image->colors;
    //             fill_mcu(image, mcu, s, mcu_starting_position);
    //             traitement_mcu(mcu, blocs, s, nbr_bloc_mcu, cos_table, colors, &tmp_ycbcr, tmp_blocs);
    //             add_JPEG_total_bitstream_progressif(f_ecrire, nbr_bloc_mcu, blocs, debut[scan], fin[scan], scan);
    //         }
    //     }

    //     /* on ecrit "EOI" que vers la fin */
    //     add_JPEG_end_progressif(f_ecrire, scan, SCANS);
    // }

    clock_gettime(CLOCK_MONOTONIC, &t_end);
    double elapsed = (t_end.tv_sec - t_start.tv_sec) + (t_end.tv_nsec - t_start.tv_nsec) / 1e9;
    printf("Encoding time: %.4f s\n", elapsed);

    free(tmp_ycbcr.Y); free(tmp_ycbcr.Cb); free(tmp_ycbcr.Cr);
    free(tmp_blocs);

    free_all(image, mcu);

    free(cos_table);
    free(blocs);

  
    fclose(f_ecrire);
}
