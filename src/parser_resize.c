#include "parser_resize.h"

/*
    - lire les images de puis les fichiers .pgm et .ppm  
    - constuire la structure "Image", avec :
        -> data : un pointeur vers un tableau des valeurs des pixels  
*/

// fread(mcu, 1, h[0]*8, f)
// fread(NULL, 1, 2*h[0]*8, f)

/* image sans colors */
void read_pgm(FILE *f, int width, int height, Image *image) {
    image->width = width;
    image->height = height;
    image->colors = 1;
    image->data = malloc(width * height);

    if (fread(image->data, 1, width * height, f) != (size_t)width * height) {
        fprintf(stderr, "ERREUR : fread failed\n");
    }
}

/* image avec colors */
void read_ppm(FILE *f, int width, int height, Image *image) {
    image->width = width;
    image->height = height;
    image->colors = 3;
    image->data = malloc(width * height * 3);

    if (fread(image->data, sizeof(*image->data), width * height * 3, f) != (size_t)width * height * 3 * sizeof(*image->data)) {
        fprintf(stderr, "ERREUR : fread failed\n");
    }
}

/* lire l'image et adapter ses dimensions */
void read_file(FILE *f, Image *image) {
    char magic[3];
    int width, height, maxval;

    if (fscanf(f, "%2s", magic) != 1) {
        fclose(f);
        exit(EXIT_FAILURE);
    }
    if (fscanf(f, "%d %d", &width, &height) != 2) {
        fclose(f);
        exit(EXIT_FAILURE);
    }
    if (fscanf(f, "%d", &maxval) != 1) {
        fclose(f);
        exit(EXIT_FAILURE);
    }
    fgetc(f);

    if (strcmp(magic, "P5") == 0) {
        read_pgm(f, width, height, image);
    } else if (strcmp(magic, "P6") == 0) {
        read_ppm(f, width, height, image);
    } else {
        fprintf(stderr, "ERREUR : unsupported format (P5/P6 expected)\n");
        fclose(f);
        exit(EXIT_FAILURE);
    }

    fclose(f);
}

void write_pixel(Image *old, Image *new, int new_x, int new_y, int old_x, int old_y) {
    if (new->colors == 1) {
        PIX(new, new_x, new_y) = PIX(old, old_x, old_y);
    } else {
        R(new, new_x, new_y) = R(old, old_x, old_y);
        G(new, new_x, new_y) = G(old, old_x, old_y);
        B(new, new_x, new_y) = B(old, old_x, old_y);
    }
}

void refill(Image *old, Image *new) {
    for (int y = 0; y < new->height; y++) {
        for (int x = 0; x < new->width; x++) {
            if (y < old->height && x < old->width) {
                write_pixel(old, new, x, y, x, y);
            } else if (y < old->height) {
                write_pixel(old, new, x, y, old->width - 1, y);
            } else if (x < old->width) {
                write_pixel(old, new, x, y, x, old->height - 1);
            } else {
                write_pixel(old, new, x, y,old->width - 1, old->height - 1);
            }
        }
    }
}

/* width et height multiple de 8 * h1 et 8 * v1 */
void resize(Image *old, Image *new, sampling_factors s) {

    int mcu_w = s.h[0] * 8;
    int mcu_h = s.v[0] * 8;

    int reste_h = old->width  % mcu_w;
    int reste_v = old->height % mcu_h;

    int nbr_colonnes_a_ajouter = (reste_h == 0) ? 0 : mcu_w - reste_h;
    int nbr_lignes_a_ajouter   = (reste_v == 0) ? 0 : mcu_h - reste_v;

    new->width = old->width + nbr_colonnes_a_ajouter;
    new->height = old->height + nbr_lignes_a_ajouter;
    printf("%d %d -> %d %d\n", old->width, old->height, new->width, new->height);
    new->colors = old->colors;
    new->data = malloc(new->width * new->height * new->colors);

    refill(old, new);
}


