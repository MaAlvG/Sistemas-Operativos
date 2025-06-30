#include <stdio.h>
#include <stdlib.h>
#include <png.h>
#include <string.h>
#include "bwfs_image.h"

// Función auxiliar para manejar errores de libpng
void user_error_fn(png_structp png_ptr, png_const_charp error_msg) {
    fprintf(stderr, "libpng error: %s\n", error_msg);
    longjmp(png_jmpbuf(png_ptr), 1);
}

int create_bw_image(const char *filepath, int width, int height) {
    FILE *fp = fopen(filepath, "wb");
    if (!fp) return -1;

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, user_error_fn, NULL);
    if (!png_ptr) {
        fclose(fp);
        return -1;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, NULL);
        fclose(fp);
        return -1;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);
        return -1;
    }

    png_init_io(png_ptr, fp);

    png_set_IHDR(png_ptr, info_ptr, width, height, 1, PNG_COLOR_TYPE_GRAY, 
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png_ptr, info_ptr);

    // Crear una fila de píxeles negros (0)
    png_bytep row = (png_bytep)malloc(png_get_rowbytes(png_ptr, info_ptr));
    memset(row, 0, png_get_rowbytes(png_ptr, info_ptr));

    for (int y = 0; y < height; y++) {
        png_write_row(png_ptr, row);
    }

    free(row);
    png_write_end(png_ptr, NULL);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);

    return 0;
}

int write_data_to_image(const char *filepath, const unsigned char *data, size_t size) {
    // Esta es una implementación simplificada. Primero leemos la imagen, la modificamos y la reescribimos.
    // Una implementación más eficiente modificaría el archivo directamente si es posible.
    
    FILE *fp = fopen(filepath, "rb");
    if (!fp) return -1;

    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, user_error_fn, NULL);
    if (!png_ptr) { fclose(fp); return -1; }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) { png_destroy_read_struct(&png_ptr, NULL, NULL); fclose(fp); return -1; }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return -1;
    }

    png_init_io(png_ptr, fp);
    png_read_info(png_ptr, info_ptr);

    int width = png_get_image_width(png_ptr, info_ptr);
    int height = png_get_image_height(png_ptr, info_ptr);
    size_t rowbytes = png_get_rowbytes(png_ptr, info_ptr);

    if ((size_t)(width * height / 8) < size) {
        fprintf(stderr, "Error: Not enough space in image for data.\n");
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return -1;
    }

    png_bytep *row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * height);
    for (int y = 0; y < height; y++) {
        row_pointers[y] = (png_byte *)malloc(rowbytes);
    }
    png_read_image(png_ptr, row_pointers);
    fclose(fp);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

    // Modificar los datos de la imagen con el buffer de entrada
    for (size_t i = 0; i < size; i++) {
        for (int bit = 0; bit < 8; bit++) {
            int pixel_index = i * 8 + bit;
            int y = pixel_index / width;
            int x = pixel_index % width;
            
            if (y < height) {
                png_bytep row = row_pointers[y];
                if ((data[i] >> (7 - bit)) & 1) {
                    row[x / 8] |= (1 << (7 - (x % 8)));
                } else {
                    row[x / 8] &= ~(1 << (7 - (x % 8)));
                }
            }
        }
    }

    // Escribir la imagen modificada
    fp = fopen(filepath, "wb");
    if (!fp) { /* free memory */ return -1; }

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, user_error_fn, NULL);
    if (!png_ptr) { /* ... */ return -1; }

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) { /* ... */ return -1; }

    if (setjmp(png_jmpbuf(png_ptr))) { /* ... */ return -1; }

    png_init_io(png_ptr, fp);
    png_set_IHDR(png_ptr, info_ptr, width, height, 1, PNG_COLOR_TYPE_GRAY, 
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png_ptr, info_ptr);
    png_write_image(png_ptr, row_pointers);
    png_write_end(png_ptr, NULL);

    // Liberar memoria
    for (int y = 0; y < height; y++) {
        free(row_pointers[y]);
    }
    free(row_pointers);

    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);

    return 0;
}

int read_data_from_image(const char *filepath, unsigned char *data, size_t size) {
    FILE *fp = fopen(filepath, "rb");
    if (!fp) return -1;

    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, user_error_fn, NULL);
    if (!png_ptr) { fclose(fp); return -1; }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) { png_destroy_read_struct(&png_ptr, NULL, NULL); fclose(fp); return -1; }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return -1;
    }

    png_init_io(png_ptr, fp);
    png_read_info(png_ptr, info_ptr);

    int width = png_get_image_width(png_ptr, info_ptr);
    int height = png_get_image_height(png_ptr, info_ptr);
    size_t rowbytes = png_get_rowbytes(png_ptr, info_ptr);

    size_t max_bytes = width * height / 8;
    if (size < max_bytes) {
        fprintf(stderr, "Warning: buffer is smaller than image capacity.\n");
    } else {
        size = max_bytes;
    }

    png_bytep *row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * height);
    for (int y = 0; y < height; y++) {
        row_pointers[y] = (png_byte *)malloc(rowbytes);
    }
    png_read_image(png_ptr, row_pointers);
    fclose(fp);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

    // Convertir píxeles a buffer de datos
    memset(data, 0, size);
    for (size_t i = 0; i < size; i++) {
        for (int bit = 0; bit < 8; bit++) {
            int pixel_index = i * 8 + bit;
            int y = pixel_index / width;
            int x = pixel_index % width;

            if (y < height) {
                png_bytep row = row_pointers[y];
                if ((row[x / 8] >> (7 - (x % 8))) & 1) {
                    data[i] |= (1 << (7 - bit));
                }
            }
        }
    }

    // Liberar memoria
    for (int y = 0; y < height; y++) {
        free(row_pointers[y]);
    }
    free(row_pointers);

    return size;
}
