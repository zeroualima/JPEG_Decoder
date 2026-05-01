#include "decoupe_mcu.h"

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

Image *resize(Image *old, int mcu_h, int mcu_v, int block_h, int block_v) {
    int reste_h = old->width % (block_h * mcu_h);
    int nbr_clonnes_a_ajouter = (reste_h == 0) ? 0 : block_h * mcu_h - reste_h;

    int reste_v = old->height % (block_v * mcu_v);
    int nbr_lignes_a_ajouter = (reste_v == 0) ? 0 : block_v * mcu_v - reste_v;

    Image *new = malloc(sizeof(Image));
    new->width = old->width + nbr_clonnes_a_ajouter;
    new->height = old->height + nbr_lignes_a_ajouter;
    // printf("%d %d -> %d %d\n", old->width, old->height, new->width, new->height);
    new->colors = old->colors;
    new->data = malloc(new->width * new->height * new->colors);

    refill(old, new);

    free(old->data);
    free(old);

    return new;
}