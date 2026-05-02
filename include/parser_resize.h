#ifndef _PARSER_RESIZE_H_
#define _PARSER_RESIZE_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define PIX(img, x, y) ((img)->data[(y)*(img)->width + (x)])
#define R(img, x, y) ((img)->data[3*((y)*(img)->width + (x)) + 0])
#define G(img, x, y) ((img)->data[3*((y)*(img)->width + (x)) + 1])
#define B(img, x, y) ((img)->data[3*((y)*(img)->width + (x)) + 2])

/* structures */
typedef struct {
    int height;
    int width;
    int colors;     /* 1 for Gray, 3 for RGB */
    uint8_t *data;
} Image;

typedef struct {
    uint8_t h[3];   /* [Y, Cb, Cr] */
    uint8_t v[3];   /* [Y, Cb, Cr] */
} sampling_factors;

/* prototypes */
void read_pgm(FILE *f, int width, int height, Image *image);
void read_ppm(FILE *f, int width, int height, Image *image);
void read_file(FILE *f, Image *image);
void resize(Image *old, Image *new, sampling_factors s);


#endif 