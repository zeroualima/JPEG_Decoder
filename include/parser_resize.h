#ifndef _PARSER_RESIZE_H_
#define _PARSER_RESIZE_H_

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <math.h>
#include <string.h>
#include <ctype.h>

#define Max(x,y) ((x>y)?x:y)

/* structures */
typedef struct {
    uint8_t h[3];   /* [Y, Cb, Cr] */
    uint8_t v[3];   /* [Y, Cb, Cr] */
} sampling_factors;

typedef int16_t RGB[3];

typedef struct {
    RGB *data;
} rgb_mcu;

typedef struct {
    char magic[3];                  // the magic nubmer : the version : p5 or p6
    int maxval; 
    FILE *f;                        //the image file pointer 
    long header_offset;
    int w;                          //width
    int h;                          //heigth
    int pw;                         //padded heigth
    int ph;                         // padded width : multiple of 8*v[0]
    long *mcus_starting_position;   //an array containing the position to start a content of an mcu : measured in bytes
    int mcu_count;                  // how many mcu we've got
    int colors;                     // 1 for PGM (P5), 3 for PPM (P6)
    rgb_mcu current_mcu;            // points  to the current mcu
} Image;

/* prototypes */
void init_mcu(rgb_mcu *m, sampling_factors s);
void init_image(Image *image, char *path, sampling_factors s);
void fill_mcu(Image *image, rgb_mcu *m, sampling_factors s, long start_position);
void free_all(Image *image, rgb_mcu *m);

#endif  /* _PARSER_RESIZE_H_ */