#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <png.h>
#include "extract.h"

void sobel(unsigned char input[HEIGHT][WIDTH], unsigned char output[HEIGHT][WIDTH]) {
    int gx, gy;
    int sobel_x[3][3] = {
        {-1, 0, 1},
        {-2, 0, 2},
        {-1, 0, 1}
    };
    int sobel_y[3][3] = {
        {-1, -2, -1},
        { 0,  0,  0},
        { 1,  2,  1}
    };

    for (int y = 1; y < HEIGHT - 1; y++) {
        for (int x = 1; x < WIDTH - 1; x++) {
            gx = 0;
            gy = 0;
            for (int i = -1; i <= 1; i++) {
                for (int j = -1; j <= 1; j++) {
                    gx += input[y + i][x + j] * sobel_x[i + 1][j + 1];
                    gy += input[y + i][x + j] * sobel_y[i + 1][j + 1];
                }
            }
            output[y][x] = (unsigned char)(sqrt(gx * gx + gy * gy) > 255 ? 255 : sqrt(gx * gx + gy * gy));
        }
    }
}

void read_png_file(const char *filename, png_bytep **row_pointers, int *width, int *height) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("ファイルを開けませんでした");
        exit(EXIT_FAILURE);
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) {
        perror("png_create_read_struct 失敗");
        exit(EXIT_FAILURE);
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        perror("png_create_info_struct 失敗");
        exit(EXIT_FAILURE);
    }

    if (setjmp(png_jmpbuf(png))) {
        perror("エラーが発生しました");
        exit(EXIT_FAILURE);
    }

    png_init_io(png, fp);
    png_read_info(png, info);

    *width = png_get_image_width(png, info);
    *height = png_get_image_height(png, info);
    png_byte color_type = png_get_color_type(png, info);
    png_byte bit_depth = png_get_bit_depth(png, info);

    if (bit_depth == 16)
        png_set_strip_16(png);

    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png);

    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png);

    if (png_get_valid(png, info, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png);

    if (color_type == PNG_COLOR_TYPE_RGB ||
        color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

    if (color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png);

    png_read_update_info(png, info);

    *row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * (*height));
    for (int y = 0; y < *height; y++) {
        (*row_pointers)[y] = (png_byte *)malloc(png_get_rowbytes(png, info));
    }

    png_read_image(png, *row_pointers);

    fclose(fp);
    png_destroy_read_struct(&png, &info, NULL);
}

void write_png_file(const char *filename, png_bytep *row_pointers, int width, int height) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        perror("ファイルを開けませんでした");
        exit(EXIT_FAILURE);
    }

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) {
        perror("png_create_write_struct 失敗");
        exit(EXIT_FAILURE);
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        perror("png_create_info_struct 失敗");
        exit(EXIT_FAILURE);
    }

    if (setjmp(png_jmpbuf(png))) {
        perror("エラーが発生しました");
        exit(EXIT_FAILURE);
    }

    png_init_io(png, fp);

    png_set_IHDR(
        png,
        info,
        width, height,
        8,
        PNG_COLOR_TYPE_RGBA,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );
    png_write_info(png, info);

    png_write_image(png, row_pointers);
    png_write_end(png, NULL);

    for (int y = 0; y < height; y++) {
        free(row_pointers[y]);
    }
    free(row_pointers);

    fclose(fp);
    png_destroy_write_struct(&png, &info);
}

void convert_to_grayscale(png_bytep *row_pointers, int width, int height, unsigned char image[HEIGHT][WIDTH]) {
for (int y = 0; y < height; y++) {
    png_bytep row = row_pointers[y];
    for (int x = 0; x < width; x++) {
        png_bytep px = &(row[x * 4]);
        unsigned char red = *px;
        unsigned char green = *px;
        unsigned char blue = *px;
        unsigned char gray = (unsigned char)(0.299 * red + 0.587 * green + 0.114 * blue);
        image[y][x] = gray;
    }
}
}

void convert_to_rgba(unsigned char image[HEIGHT][WIDTH], png_bytep *row_pointers, int width, int height) {
    for (int y = 0; y < height; y++) {
        png_bytep row = row_pointers[y];
        for (int x = 0; x < width; x++) {
            png_bytep px = &(row[x * 4]);
            *px = image[y][x];
            *(px+1) = image[y][x];
            *(px+2) = image[y][x];
            *(px+3) = 255;
            if((*px >= WHITE_RANGE && (*px+1) >= WHITE_RANGE && (*px+2) >= WHITE_RANGE))
            {
                *px = 255;
                *(px+1) = 255;
                *(px+2) = 255;
                *(px+3) = 255;
            }
        }
    }
}

int main() {
    const char *input_file = "test.png";
    const char *output_file = "test2.png";

    png_bytep *row_pointers;
    int width, height;

    unsigned char input[HEIGHT][WIDTH];
    unsigned char output[HEIGHT][WIDTH];

    read_png_file(input_file, &row_pointers, &width, &height);
    convert_to_grayscale(row_pointers, width, height, input);
    sobel(input, output);
    convert_to_rgba(output, row_pointers, width, height);
    write_png_file(output_file, row_pointers, width, height);

    return 0;
}
