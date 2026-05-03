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

/* read one pixel from f into dst, handling PGM (colors=1) and PPM (colors=3),
   8-bit and 16-bit. PGM gray value is replicated to all 3 channels. */
static void read_pixel(RGB dst, int maxval, int colors, FILE *f) {
    if (colors == 1) {
        if (maxval <= 255) {
            uint8_t g;
            if (fread(&g, 1, 1, f) != (size_t)1) {
                fprintf(stderr, "ERREUR : fread failed\n");
            }
            dst[0] = dst[1] = dst[2] = g;
        } else {
            uint8_t buf[2];
            if (fread(buf, 1, 2, f) != (size_t)2) {
                fprintf(stderr, "ERREUR : fread failed\n");
            }
            dst[0] = dst[1] = dst[2] = (int16_t)((buf[0] << 8) | buf[1]);
        }
    } else {
        if (maxval <= 255) {
            uint8_t buf[3];
            if (fread(buf, 1, 3, f) != (size_t)3) {
                fprintf(stderr, "ERREUR : fread failed\n");
            }
            dst[0] = buf[0];
            dst[1] = buf[1];
            dst[2] = buf[2];
        } else {
            uint8_t buf[6];
            if (fread(buf, 1, 6, f) != (size_t)6) {
                fprintf(stderr, "ERREUR : fread failed\n");
            }
            dst[0] = (int16_t)((buf[0] << 8) | buf[1]);
            dst[1] = (int16_t)((buf[2] << 8) | buf[3]);
            dst[2] = (int16_t)((buf[4] << 8) | buf[5]);
        }
    }
}

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
    int pixel_col_start = (int)(((start_position - image->header_offset) % row_stride) / bytes_pp);

    for (int i = 0; i < mcu_height; i++) {
        long file_row_offset = start_position + i * row_stride;
        long pixel_row       = (file_row_offset - image->header_offset) / row_stride;

        if (pixel_row < image->h) {
            fseek(image->f, file_row_offset, SEEK_SET);
            for (int j = 0; j < mcu_width; j++) {
                if (pixel_col_start + j < image->w) {
                    read_pixel(m->data[i * mcu_width + j], image->maxval, image->colors, image->f);
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
    free(image->mcus_starting_position);
    free(m->data);
    fclose(image->f);
}
