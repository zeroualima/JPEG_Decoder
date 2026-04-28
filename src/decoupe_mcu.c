#include "decoupe_mcu.h"

Image *resize(Image *old, int mcu_h, int mcu_v, int block_h, int block_v) {
    int reste_h = old->width % (block_h * mcu_h);
    int nbr_clonnes_a_ajouter = block_h * mcu_h - reste_h;

    int reste_v = old->height % (block_v * mcu_v);
    int nbr_lignes_a_ajouter = block_v * mcu_v - reste_v;

    Image *new = malloc(sizeof(Image));
    new->width = old->width + nbr_clonnes_a_ajouter;
    new->height = old->height + nbr_lignes_a_ajouter;
    new->colors = old->colors;
    new->data = malloc(new->width * new->height * new->colors);

    for (int y = 0; y < new->height; y++) {
        for (int x = 0; x < new->width; x++) {
            if (y < old->height && x < old->width) {
                if (new->colors == 1) {
                    PIX(new, x, y) = PIX(old, x, y);
                } else {
                    R(new, x, y) = R(old, x, y);
                    G(new, x, y) = G(old, x, y);
                    B(new, x, y) = B(old, x, y);
                }
            } else if (y < old->height) {
                if (new->colors == 1) {
                    PIX(new, x, y) = PIX(old, old->width - 1, y);
                } else {
                    R(new, x, y) = R(old, old->width - 1, y);
                    G(new, x, y) = G(old, old->width - 1, y);
                    B(new, x, y) = B(old, old->width - 1, y);
                }
            } else {
                if (new->colors == 1) {
                    PIX(new, x, y) = PIX(old, x, old->height - 1);
                } else {
                    R(new, x, y) = R(old, x, old->height - 1);
                    G(new, x, y) = G(old, x, old->height - 1);
                    B(new, x, y) = B(old, x, old->height - 1);
                }
            }
        }
    }

    // free(old->data);
    // free(old);

    return new;
}