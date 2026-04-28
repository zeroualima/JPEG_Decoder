#include "parser.h"
#include "decoupe_mcu.h"

Image *pgm_parser(FILE *f, int width, int height) {

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

Image *ppm_parser(FILE *f, int width, int height) {

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

int read_file(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return 1;
    } 

    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        perror("fopen");
        return 1;
    }

    char magic[3];
    int width, height, maxval;

    if (fscanf(f, "%2s", magic) != 1) return 1;
    if (fscanf(f, "%d %d", &width, &height) != 2) return 1;
    if (fscanf(f, "%d", &maxval) != 1) return 1;

    fgetc(f);

    if (strcmp(magic, "P5") == 0) {
        Image *image = pgm_parser(f, width, height);
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                printf("%3d ", PIX(image, x, y));
            }
            printf("\n");
        }
        Image *resized_img = resize(image, 1, 1, 5, 5);
        for (int y = 0; y < resized_img->height; y++) {
            for (int x = 0; x < resized_img->width; x++) {
                printf("%3d ", PIX(resized_img, x, y));
            }
            printf("\n");
        }
        free(resized_img->data);
        free(resized_img);
    } else if (strcmp(magic, "P6") == 0) {
        Image *image = ppm_parser(f, width, height);for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                printf("[%d, %d, %d]", R(image, x, y), B(image, x, y), G(image, x, y));
            }
            printf("\n");
        }
        free(image->data);
        free(image);
    } else {
        fprintf(stderr, "Unsupported format (P5/P6 expected)\n");
        fclose(f);
        return 1;
    }

    fclose(f);
    printf("\n");
    return 0;
}