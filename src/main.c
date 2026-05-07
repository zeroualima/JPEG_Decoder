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
    char *sampling_arg = NULL;

    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--help", 6) == 0) {
            printf("Usage: %s [--outfile=<output.jpg>] [--sample=h1xv1,h2xv2,h3xv3] <input>\n", argv[0]);
            return 0;
        } else if (strncmp(argv[i], "--outfile=", 10) == 0) {
            outfile_arg = argv[i] + 10;
        } else if (strncmp(argv[i], "--sample=", 9) == 0) {
            sampling_arg = argv[i] + 9;
        } else {
            infile = argv[i];
        }
    }

    if (!infile) {
        fprintf(stderr, "Usage: %s [--outfile=<output.jpg>] [--sample=h1xv1,h2xv2,h3xv3] <input>\n", argv[0]);
        exit(1);
    }

    FILE *f_lire = fopen(infile, "rb");
    if (!f_lire) {
        perror("fopen lecture");
        exit(1);
    }

    char *slash = strrchr(infile, '/');
    char *filename = slash ? slash + 1 : infile;
    char *dot = strrchr(filename, '.');
    if (!dot) {
        fprintf(stderr, "Erreur : pas d'extension trouvée dans '%s'\n", infile);
        fclose(f_lire);
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
        fclose(f_lire);
        if (output_path_allocated) free(output_path);
        exit(1);
    }
    if (output_path_allocated) 
        free(output_path);

    // Sampling
    sampling_factors s;
    if (nb_colors == 1) {
        s.h[0] = 1; s.v[0] = 1;
        s.h[1] = 1; s.v[1] = 1;
        s.h[2] = 1; s.v[2] = 1;
    } else if (sampling_arg) {
        s.h[0] = sampling_arg[0] - '0'; s.v[0] = sampling_arg[2] - '0';
        s.h[1] = sampling_arg[4] - '0'; s.v[1] = sampling_arg[6] - '0';
        s.h[2] = sampling_arg[8] - '0'; s.v[2] = sampling_arg[10] - '0';
    } else {
        s.h[0] = 1; s.v[0] = 1;
        s.h[1] = 1; s.v[1] = 1;
        s.h[2] = 1; s.v[2] = 1;
    }
    test_sampling_factors(s);

    // Lecture
    
    Image *image = malloc(sizeof(Image));
    init_image(image, infile, s);

    // Ecriture
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

    // add_JPEG_entete(f_ecrire, image->h, image->w, nb_colors, s);
    // for (int i = 0; i < image->mcu_count; i++) {
    //     fill_mcu(image, mcu, s, (image->mcus_starting_position)[i]);
    //     traitement_mcu(mcu, blocs, s, nbr_bloc_mcu, cos_table, colors, &tmp_ycbcr, tmp_blocs);
    //     add_JPEG_total_bitstream(f_ecrire, nbr_bloc_mcu, blocs);
    // }
    // add_JPEG_end(f_ecrire);

    #define SCANS 5
    int debut[SCANS] = {0, 1, 1, 1, 9};
    int fin[SCANS] = {0, 8, 63, 63, 63};
    add_JPEG_entete_progressif(f_ecrire, image->h, image->w, nb_colors, s);
    for (int scan = 0; scan < SCANS; scan++) {
        if (nb_colors == 1) {
            write_SOS(f_ecrire, 1, 0, 0, 0, 0, debut[scan], fin[scan], 0); // Spectral selection 
        } else {
            write_SOS(f_ecrire, 3, 0, 0, 1, 1, debut[scan], fin[scan], 0); // Spectral selection
        }
        for (int i = 0; i < image->mcu_count; i++) {
            fill_mcu(image, mcu, s, (image->mcus_starting_position)[i]);
            traitement_mcu(mcu, blocs, s, nbr_bloc_mcu, cos_table, colors, &tmp_ycbcr, tmp_blocs);
            add_JPEG_total_bitstream_progressif(f_ecrire, nbr_bloc_mcu, blocs, debut[scan], fin[scan], scan + 1);
        }
        add_JPEG_end(f_ecrire);
    }

    clock_gettime(CLOCK_MONOTONIC, &t_end);
    double elapsed = (t_end.tv_sec - t_start.tv_sec) + (t_end.tv_nsec - t_start.tv_nsec) / 1e9;
    printf("Encoding time: %.4f s\n", elapsed);

    free(tmp_ycbcr.Y); free(tmp_ycbcr.Cb); free(tmp_ycbcr.Cr);
    free(tmp_blocs);

    free(mcu->data);
    free(mcu);

    free(image->row_buffer);
    free(image->current_mcu.data);
    free(image->mcus_starting_position);
    free(image);

    free(cos_table);
    free(blocs);

    fclose(f_lire);
    fclose(f_ecrire);
    return 0;
}
