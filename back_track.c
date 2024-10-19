#include <stdio.h>
#include <stdlib.h>
#include <png.h>
#include "extract.h"

typedef struct {
    int x, y;
} Point;

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

void flood_fill(png_bytep *row_pointers, int x, int y, int width, int height, unsigned char r, unsigned char g, unsigned char b, int **visited) {
    Point *stack;

    stack = (Point *)malloc(sizeof(Point)*(width * height));
    int top = -1;

    stack[++top] = (Point){x, y};

    while (top >= 0) {
        Point p = stack[top--];

        if (p.x < 0 || p.x >= width || p.y < 0 || p.y >= height) continue;
        if (visited[p.y][p.x]) continue;

        png_bytep px = &(row_pointers[p.y][p.x * 4]);
        if (*px == 255 && *(px+1) == 255 && *(px+2) == 255) {
            *px = r;
            *(px+1) = g;
            *(px+2) = b;
            visited[p.y][p.x] = 1;

            stack[++top] = (Point){p.x + 1, p.y};
            stack[++top] = (Point){p.x - 1, p.y};
            stack[++top] = (Point){p.x, p.y + 1};
            stack[++top] = (Point){p.x, p.y - 1};
        }
    }
    free(stack);
}

void process_image(png_bytep *row_pointers, int width, int height) {
    int **visited = (int **)malloc(height * sizeof(int *));
    for (int y = 0; y < height; y++) {
        visited[y] = (int *)calloc(width, sizeof(int));
    }

    unsigned char r = 0, g = 150, b = 0;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (!visited[y][x]) {
                png_bytep px = &(row_pointers[y][x * 4]);
                if (*px == 255 && *(px+1) == 255 && *(px+2) == 255) {
                    flood_fill(row_pointers, x, y, width, height, r, g, b, visited);
                    r = (r + 100) % 256;
                    b = (b + 20) % 256;
                }
            }
        }
    }

    for (int y = 0; y < height; y++) {
        free(visited[y]);
    }
    free(visited);
}

int main() {
    const char *input_file = "test2.png";
    const char *output_file = "test4.png";

    png_bytep *row_pointers;
    int width, height;

    read_png_file(input_file, &row_pointers, &width, &height);
    process_image(row_pointers, width, height);
    write_png_file(output_file, row_pointers, width, height);

    return 0;
}
