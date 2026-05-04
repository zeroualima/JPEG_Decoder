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

/* bytes per pixel: colors for 8-bit, colors*2 for 16-bit */
static inline int bpp(int maxval, int colors) { return (maxval <= 255) ? colors : colors * 2; }

void detect_mcu(Image *image, sampling_factors s);

void init_image(Image *image, char *path, sampling_factors s) {
    image->f = fopen(path, "rb");

    if (!fscanf(image->f, "%2s", image->magic)) {
        fclose(image->f);
        exit(EXIT_FAILURE);
    }
    skip_whitespace_and_comments(image->f);
    if (!fscanf(image->f, "%d", &image->w)) {
        fclose(image->f);
        exit(EXIT_FAILURE);
    }
    skip_whitespace_and_comments(image->f);
    if (!fscanf(image->f, "%d", &image->h)) {
        fclose(image->f);
        exit(EXIT_FAILURE);
    }
    skip_whitespace_and_comments(image->f);
    if (!fscanf(image->f, "%d", &image->maxval)) {
        fclose(image->f);
        exit(EXIT_FAILURE);
    }
    fgetc(image->f);

    image->header_offset = ftell(image->f);
    image->colors = (image->magic[1] == '5') ? 1 : 3;

    int mcu_h = 8 * s.v[0];
    int buf_size = mcu_h * image->w * bpp(image->maxval, image->colors);
    image->row_buffer = malloc(buf_size);
    image->row_buffer_pixel_start = -1;
    image->row_buffer_height = mcu_h;

    int unit_w = 8 * s.h[0];
    int unit_h = 8 * s.v[0];
    image->pw = ((image->w + unit_w - 1) / unit_w) * unit_w;
    image->ph = ((image->h + unit_h - 1) / unit_h) * unit_h;

    detect_mcu(image, s);
}

void detect_mcu(Image *image, sampling_factors s) {
    int mcu_rows = image->ph / (8 * s.v[0]);
    int mcu_cols = image->pw / (8 * s.h[0]);
    image->mcu_count = mcu_rows * mcu_cols;

    image->mcus_starting_position = malloc(sizeof(long) * image->mcu_count);

    int k = 0;
    for (int i = 0; i < mcu_rows; i++) {
        for (int j = 0; j < mcu_cols; j++) {
            int pixel_row = i * 8 * s.v[0];
            int pixel_col = j * 8 * s.h[0];
            image->mcus_starting_position[k++] = image->header_offset + (long)(pixel_row * image->w + pixel_col) * bpp(image->maxval, image->colors);
        }
    }

    init_mcu(&image->current_mcu, s);
}

void fill_mcu(Image *image, rgb_mcu *m, sampling_factors s, long start_position) {
    int bytes_pp        = bpp(image->maxval, image->colors);
    int row_stride      = image->w * bytes_pp;
    int mcu_width       = 8 * s.h[0];
    int mcu_height      = 8 * s.v[0];
    long data_offset    = start_position - image->header_offset;
    int pixel_row_start = (int)(data_offset / row_stride);
    int pixel_col_start = (int)((data_offset % row_stride) / bytes_pp);

    /* load this MCU row's pixel rows if not already buffered */
    if (image->row_buffer_pixel_start != pixel_row_start) {
        int rows_to_read = mcu_height;
        if (pixel_row_start + rows_to_read > image->h)
            rows_to_read = image->h - pixel_row_start;
        long file_offset = image->header_offset + (long)pixel_row_start * row_stride;
        fseek(image->f, file_offset, SEEK_SET);
        if (fread(image->row_buffer, bytes_pp, rows_to_read * image->w, image->f) == 0)
            fprintf(stderr, "ERREUR : fread row buffer failed\n");
        image->row_buffer_pixel_start = pixel_row_start;
    }

    for (int i = 0; i < mcu_height; i++) {
        int pixel_row = pixel_row_start + i;
        if (pixel_row < image->h) {
            for (int j = 0; j < mcu_width; j++) {
                int col = pixel_col_start + j;
                if (col < image->w) {
                    uint8_t *src = image->row_buffer + (i * image->w + col) * bytes_pp;
                    if (image->colors == 1) {
                        int16_t v = (image->maxval <= 255) ? src[0] : (int16_t)((src[0] << 8) | src[1]);
                        m->data[i * mcu_width + j][0] = v;
                        m->data[i * mcu_width + j][1] = v;
                        m->data[i * mcu_width + j][2] = v;
                    } else {
                        if (image->maxval <= 255) {
                            m->data[i * mcu_width + j][0] = src[0];
                            m->data[i * mcu_width + j][1] = src[1];
                            m->data[i * mcu_width + j][2] = src[2];
                        } else {
                            m->data[i * mcu_width + j][0] = (int16_t)((src[0] << 8) | src[1]);
                            m->data[i * mcu_width + j][1] = (int16_t)((src[2] << 8) | src[3]);
                            m->data[i * mcu_width + j][2] = (int16_t)((src[4] << 8) | src[5]);
                        }
                    }
                } else {
                    m->data[i * mcu_width + j][0] = m->data[i * mcu_width + j - 1][0];
                    m->data[i * mcu_width + j][1] = m->data[i * mcu_width + j - 1][1];
                    m->data[i * mcu_width + j][2] = m->data[i * mcu_width + j - 1][2];
                }
            }
        } else {
            if (i > 0)
                memcpy(m->data[i * mcu_width], m->data[(i - 1) * mcu_width],
                       sizeof(RGB) * mcu_width);
        }
    }
}

void free_all(Image *image, rgb_mcu *m) {
    free(image->row_buffer);
    free(image->mcus_starting_position);
    free(m->data);
    fclose(image->f);
}
