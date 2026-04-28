#ifndef PARSER_H
#define PARSER_H

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define PIX(img, x, y) ((img)->data[(y)*(img)->width + (x)])
#define R(img, x, y) ((img)->data[3*((y)*(img)->width + (x)) + 0])
#define G(img, x, y) ((img)->data[3*((y)*(img)->width + (x)) + 1])
#define B(img, x, y) ((img)->data[3*((y)*(img)->width + (x)) + 2])

typedef struct {
    int width;
    int height;
    int colors; // 1 for Gray, 3 for RGB
    unsigned char *data;  // flat RGB or grayscale
} Image;

Image *read_pgm(FILE *f, int width, int height);
Image *read_ppm(FILE *f, int width, int height);
int read_file(int argc, char **argv);


#endif