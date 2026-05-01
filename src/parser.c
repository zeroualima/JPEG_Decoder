#include "parser.h"
#include "decoupe_mcu.h"

Image *read_pgm(FILE *f, int width, int height) {

    Image *image = malloc(sizeof(Image));
    image->width = width;
    image->height = height;
    image->colors = 1;
    image->data = malloc(width * height);

    if (fread(image->data, 1, width * height, f) != (size_t)width * height) {
        fprintf(stderr, "fread failed\n");
    }

    return image;
}

Image *read_ppm(FILE *f, int width, int height) {

    Image *image = malloc(sizeof(Image));
    image->width = width;
    image->height = height;
    image->colors = 3;
    image->data = malloc(width * height * 3);

    if (fread(image->data, sizeof(*image->data), width * height, f) != (size_t)width * height * sizeof(*image->data)) {
        fprintf(stderr, "fread failed\n");
    }

    return image;
}

Image *read_file(FILE *f, int mcu_h, int mcu_v, int block_h, int block_v) {
    char magic[3];
    int width, height, maxval;

    if (fscanf(f, "%2s", magic) != 1) return NULL;
    if (fscanf(f, "%d %d", &width, &height) != 2) return NULL;
    if (fscanf(f, "%d", &maxval) != 1) return NULL;

    fgetc(f);

    if (strcmp(magic, "P5") == 0) {
        Image *image = read_pgm(f, width, height);
        // for (int y = 0; y < height; y++) {
        //     for (int x = 0; x < width; x++) {
        //         printf("%3d ", PIX(image, x, y));
        //     }
        //     printf("\n");
        // }
        Image *resized_img = resize(image, mcu_h, mcu_v, block_h, block_v);
        // for (int y = 0; y < resized_img->height; y++) {
        //     for (int x = 0; x < resized_img->width; x++) {
        //         printf("%3d ", PIX(resized_img, x, y));
        //     }
        //     printf("\n");
        // }
        // free(image->data);
        // free(image);
        return resized_img;
    } else if (strcmp(magic, "P6") == 0) {
        Image *image = read_ppm(f, width, height);
        // for (int y = 0; y < height; y++) {
        //     for (int x = 0; x < width; x++) {
        //         printf("[%d, %d, %d]", R(image, x, y), B(image, x, y), G(image, x, y));
        //     }
        //     printf("\n");
        // }
        Image *resized_img = resize(image, mcu_h, mcu_v, block_h, block_v);
        // free(image->data);
        // free(image);
        return resized_img;
    } else {
        fprintf(stderr, "Unsupported format (P5/P6 expected)\n");
    }

    fclose(f);
    printf("\n");
    return NULL;
}