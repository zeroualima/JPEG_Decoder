#include "parser_resize.h"

void init_mcu(rgb_mcu *m, sampling_factors s) {
    m->data = calloc(8 * 8 * s.h[0] * s.v[0], sizeof(RGB));
}

void skip_whitespace_and_comments(FILE *fp) {
    int c;
    while (1) {
        c = fgetc(fp);
        if (isspace(c)) continue;
        if (c == '#') {
            while ((c = fgetc(fp)) != '\n' && c != EOF);
            continue;
        }
        ungetc(c, fp);
        break;
    }
}


void init_image(Image *image, char *path, sampling_factors s) {
    image->f = fopen(path, "rb");

    if (!fscanf(image->f, "%2s", image->magic)) {
        fclose(image->f);
        fprintf(stderr, "Pas de nombre magic dans le fichier d'entree\n");
        exit(1);
    }
    skip_whitespace_and_comments(image->f);
    if (!fscanf(image->f, "%d", &image->w)) {
        fclose(image->f);
        fprintf(stderr, "Lageur inconvenable\n");
        exit(1);
    }
    skip_whitespace_and_comments(image->f);
    if (!fscanf(image->f, "%d", &image->h)) {
        fclose(image->f);
        fprintf(stderr, "Hauteur inconvenable\n");
        exit(1);
    }
    skip_whitespace_and_comments(image->f);
    if (!fscanf(image->f, "%d", &image->maxval)) {
        fclose(image->f);
        fprintf(stderr, "Maxval inconvenable\n");
        exit(1);
    }
    fgetc(image->f);

    image->header_offset = ftell(image->f);
    image->colors = (image->magic[1] == '5') ? 1 : 3;

    int mcu_h = 8 * s.v[0];
    int buf_size = mcu_h * image->w *  image->colors;
    image->row_buffer = malloc(buf_size);
    image->row_buffer_mcu_start = -1;
    image->row_buffer_height = mcu_h;

    int unit_w = 8 * s.h[0];
    int unit_h = 8 * s.v[0];
    image->pw = ((image->w + unit_w - 1) / unit_w) * unit_w;
    image->ph = ((image->h + unit_h - 1) / unit_h) * unit_h;

}



void fill_mcu(Image *image, rgb_mcu *m, sampling_factors s, long start_position) {

    /*variables declaration : following C99 standards  */
    int bytes_per_pixel, row_stride, mcu_width, mcu_height, mcu_row_start, mcu_col_start, col;
    long data_offset, file_offset;
    uint8_t *pixel_rgb;
    int16_t y_val;


    bytes_per_pixel =  image->colors; //just renaming for clarity

    row_stride      = image->w * bytes_per_pixel; //row width measured in bytes

    /*mcu specs*/
    mcu_width   = 8 * s.h[0];
    mcu_height  = 8 * s.v[0];


    data_offset    = start_position - image->header_offset;//measure the position relative to the mcu starting position (in bytes)

    /* where do the mcu start : row and col of its first pixel*/
    mcu_row_start = (int)(data_offset / row_stride); 
    mcu_col_start = (int)((data_offset % row_stride) / bytes_per_pixel);


    /* load this MCU row's pixel rows if not already buffered */
    if (image->row_buffer_mcu_start != mcu_row_start) {//init with -1
        int rows_to_read = mcu_height;
        if (mcu_row_start + rows_to_read > image->h){
                rows_to_read = image->h - mcu_row_start;
        }
            
        file_offset = image->header_offset + (long)mcu_row_start * row_stride;
        fseek(image->f, file_offset, SEEK_SET);
        if (fread(image->row_buffer, bytes_per_pixel, rows_to_read * image->w, image->f) == 0){
                fprintf(stderr, " failed to read from the file\n");
        }
        
        image->row_buffer_mcu_start = mcu_row_start;
    }



    /*now we fill our  mcu*/

    for (int i = 0; i < mcu_height; i++) {

        int pixel_row = mcu_row_start + i;

        if (pixel_row < image->h) { //there still some rows at the bottum of the image
            for (int j = 0; j < mcu_width; j++) {
                col = mcu_col_start + j;
                if (col < image->w) { //check if there is still a column on the right
                        pixel_rgb = image->row_buffer + (i * image->w + col) * bytes_per_pixel;//in case of pgm its just the Y_value
                    if (image->colors == 1) {//pgm  
                        y_val = pixel_rgb[0]; 
                        m->data[i * mcu_width + j][0] = y_val;
                        
                    } else { //ppm
                        
                        m->data[i * mcu_width + j][0] = pixel_rgb[0];
                        m->data[i * mcu_width + j][1] = pixel_rgb[1];
                        m->data[i * mcu_width + j][2] = pixel_rgb[2];
               
                    }
                } else {//we hit the extrem right column : we copy its value
                    m->data[i * mcu_width + j][0] = m->data[i * mcu_width + j - 1][0];
                    m->data[i * mcu_width + j][1] = m->data[i * mcu_width + j - 1][1];
                    m->data[i * mcu_width + j][2] = m->data[i * mcu_width + j - 1][2];
                }
            }
        } else { //we hit the lower row : we basicaly copy the values of the last row
            if (i > 0)
                memcpy(m->data[i * mcu_width], m->data[(i - 1) * mcu_width],
                       sizeof(RGB) * mcu_width);
        }
    }
}



void free_all(Image *image, rgb_mcu *m) {
    free(image->row_buffer);
    fclose(image->f);
    free(image);
    free(m->data);
    free(m);
}