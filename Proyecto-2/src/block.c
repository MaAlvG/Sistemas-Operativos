#include "../include/bwfs.h"
#define _POSIX_C_SOURCE 200809L  // Para strdup
#include "../include/block.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <png.h>

// Contexto global de bloques
block_ctx_t block_ctx = {0};

// Contexto global de BWFS
bwfs_context_t ctx = {0};

// Duplicación segura de cadenas
static char* safe_strdup(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char *new = malloc(len);
    if (new) memcpy(new, s, len);
    return new;
}

// Inicializar la gestión de bloques
int block_init(const char *base_path, size_t total_blocks) {
    printf("Inicializando gestor de bloques con ruta: %s\n", base_path);
    
    // Validar parámetros
    if (!base_path || total_blocks == 0 || total_blocks > MAX_BLOCKS) {
        fprintf(stderr, "Error: Parámetros inválidos (ruta: %p, bloques totales: %zu)\n", 
                (void*)base_path, total_blocks);
        return -1;
    }
    
    // Inicializar contexto
    memset(&block_ctx, 0, sizeof(block_ctx));
    
    // Configurar superbloque
    memcpy(block_ctx.superblock.signature, "BWFSv1\0\0\0", 8);
    block_ctx.superblock.version = 1;
    block_ctx.superblock.block_size = BLOCK_SIZE;
    block_ctx.superblock.total_blocks = total_blocks;
    block_ctx.superblock.free_blocks = total_blocks - 2; // Reservar primeros dos bloques
    block_ctx.superblock.inode_count = 1;  // Inodo raíz
    block_ctx.superblock.root_inode = 1;
    time(&block_ctx.superblock.created);
    time(&block_ctx.superblock.modified);
    
    // Configurar seguimiento de bloques
    block_ctx.total_blocks = total_blocks;
    block_ctx.free_blocks = total_blocks - 2;  // Reservar primeros dos bloques
    
    // Duplicar ruta base
    block_ctx.base_path = safe_strdup(base_path);
    if (!block_ctx.base_path) {
        fprintf(stderr, "Error: No se pudo duplicar la ruta base\n");
        return -1;
    }
    
    // Inicializar mapa de bits de bloques
    size_t bitmap_size = (total_blocks + 7) / 8;
    block_ctx.block_bitmap = calloc(1, bitmap_size);
    if (!block_ctx.block_bitmap) {
        fprintf(stderr, "Error: No se pudo asignar el mapa de bits de bloques\n");
        free(block_ctx.base_path);
        block_ctx.base_path = NULL;
        return -1;
    }
    
    // Marcar bloques del superbloque y del inodo raíz como ocupados
    block_ctx.block_bitmap[0] = 0x03;  // Primeros dos bloques (0 y 1) están ocupados
    
    printf("Gestor de bloques inicializado exitosamente con %zu bloques\n", total_blocks);
    return 0;
}

// Liberar recursos del gestor de bloques
void block_cleanup() {
    free(block_ctx.block_bitmap);
    free(block_ctx.base_path);
    memset(&block_ctx, 0, sizeof(block_ctx));
}

// Obtener nombre de archivo para un bloque (formato binario)
static char* block_get_filename(uint32_t block_num) {
    static char filename[1024];
    snprintf(filename, sizeof(filename), "%s/block_%06u.dat", block_ctx.base_path, block_num);
    return filename;
}

// Obtener nombre de archivo para un bloque (formato PNG)
static char* block_get_png_filename(uint32_t block_num) {
    static char filename[1024];
    snprintf(filename, sizeof(filename), "%s/block_%06u.png", block_ctx.base_path, block_num);
    return filename;
}

// Guardar un bloque como imagen PNG (versión simplificada)
static int block_save_as_png(uint32_t block_num, const uint8_t *data) {
    // Primero, guardar el bloque en formato binario para verificación
    char *filename = block_get_png_filename(block_num);
    char bin_filename[256];
    snprintf(bin_filename, sizeof(bin_filename), "%s.bin", filename);
    
    FILE *bin_fp = fopen(bin_filename, "wb");
    if (bin_fp) {
        fwrite(data, 1, BLOCK_SIZE, bin_fp);
        fclose(bin_fp);
        printf("DEBUG: Bloque binario guardado en %s\n", bin_filename);
    }
    
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        perror("Error al abrir archivo PNG para escritura");
        return -1;
    }

    // Inicializar estructuras PNG
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
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

    // Configuración básica de la imagen
    png_set_IHDR(png_ptr, info_ptr, 
                1000, 1000,              // Ancho y alto
                1,                       // 1 bit por píxel
                PNG_COLOR_TYPE_GRAY,     // Escala de grises
                PNG_INTERLACE_NONE,      // Sin entrelazado
                PNG_COMPRESSION_TYPE_BASE, 
                PNG_FILTER_TYPE_BASE);

    // Escribir la cabecera
    png_write_info(png_ptr, info_ptr);
    
    // Escribir los datos fila por fila
    png_bytep row = (png_bytep)data;
    for (int y = 0; y < 1000; y++) {
        png_write_row(png_ptr, row + (y * 125));
    }

    // Finalizar
    png_write_end(png_ptr, NULL);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);
    
    return 0;
}

// Cargar un bloque desde una imagen PNG (versión simplificada)
static int block_load_from_png(uint32_t block_num, uint8_t *buffer) {
    char *filename = block_get_png_filename(block_num);
    char bin_filename[256];
    
    // Primero intentar cargar el archivo binario si existe
    snprintf(bin_filename, sizeof(bin_filename), "%s.bin", filename);
    printf("DEBUG: Intentando abrir archivo binario: %s\n", bin_filename);
    FILE *bin_fp = fopen(bin_filename, "rb");
    if (bin_fp) {
        printf("DEBUG: Archivo binario abierto correctamente, leyendo...\n");
        printf("DEBUG - Antes de fread: buffer=%p, BLOCK_SIZE=%u\n", (void*)buffer, BLOCK_SIZE);
        
        // Verificar que el buffer no sea NULL
        if (buffer == NULL) {
            printf("ERROR: El buffer de destino es NULL\n");
            fclose(bin_fp);
            return -1;
        }
        
        size_t read = fread(buffer, 1, BLOCK_SIZE, bin_fp);
        printf("DEBUG: Leídos %zu bytes de %u esperados\n", read, BLOCK_SIZE);
        
        if (ferror(bin_fp)) {
            perror("Error al leer el archivo binario");
            fclose(bin_fp);
            return -1;
        }
        
        fclose(bin_fp);
        
        if (read == BLOCK_SIZE) {
            printf("DEBUG: Bloque binario cargado exitosamente\n");
            return 0; // Éxito al cargar el binario
        } else {
            printf("ERROR: Tamaño de archivo incorrecto: %zu bytes (se esperaban %u)\n", read, BLOCK_SIZE);
        }
    } else {
        printf("DEBUG: No se pudo abrir el archivo binario: %s\n", strerror(errno));
    }
    
    // Si no hay binario o falla, intentar con PNG
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("Error al abrir archivo PNG para lectura");
        return -1;
    }

    // Verificar firma PNG
    png_byte header[8];
    if (fread(header, 1, 8, fp) != 8 || png_sig_cmp(header, 0, 8)) {
        fclose(fp);
        fprintf(stderr, "El archivo no es un PNG válido\n");
        return -1;
    }

    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        fclose(fp);
        return -1;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        fclose(fp);
        return -1;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        fclose(fp);
        return -1;
    }

    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, 8);
    png_read_info(png_ptr, info_ptr);

    // Configuración básica
    png_set_packing(png_ptr);
    png_set_packswap(png_ptr);
    
    // Leer los datos
    png_bytep row = (png_bytep)buffer;
    for (int y = 0; y < 1000; y++) {
        png_read_row(png_ptr, row + (y * 125), NULL);
    }

    png_read_end(png_ptr, NULL);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(fp);
    
    return 0;
}

// Verificar si un bloque está libre
int block_is_free(uint32_t block_num) {
    if (block_num >= block_ctx.total_blocks) {
        fprintf(stderr, "Error: Número de bloque %u fuera de rango (máx %zu)\n", 
                block_num, block_ctx.total_blocks);
        return 0;
    }
    return !(block_ctx.block_bitmap[block_num / 8] & (1 << (block_num % 8)));
}

// Marcar un bloque como ocupado
static void block_mark_used(uint32_t block_num) {
    if (block_num < block_ctx.total_blocks) {
        block_ctx.block_bitmap[block_num / 8] |= (1 << (block_num % 8));
        if (block_ctx.free_blocks > 0) {
            block_ctx.free_blocks--;
        }
    }
}

// Marcar un bloque como libre
static void block_mark_free(uint32_t block_num) {
    if (block_num < block_ctx.total_blocks) {
        block_ctx.block_bitmap[block_num / 8] &= ~(1 << (block_num % 8));
        block_ctx.free_blocks++;
    }
}

// Asignar un nuevo bloque
uint32_t block_allocate() {
    if (!block_ctx.block_bitmap) {
        fprintf(stderr, "Error: Gestor de bloques no inicializado\n");
        return 0;
    }
    
    if (block_ctx.free_blocks == 0) {
        fprintf(stderr, "Error: No hay bloques libres disponibles\n");
        return 0;
    }
    
    printf("Asignando nuevo bloque (libres: %zu/%zu)\n", 
           block_ctx.free_blocks, block_ctx.total_blocks);
    
    // Asignación simple de primer ajuste
    for (uint32_t i = 2; i < block_ctx.total_blocks; i++) {  // Saltar primeros dos bloques
        if (block_is_free(i)) {
            printf("Bloque libre encontrado: %u\n", i);
            block_mark_used(i);
            return i;
        }
    }
    
    fprintf(stderr, "Error: No se encontraron bloques libres (estado inconsistente)\n");
    return 0; // No debería llegar aquí si free_blocks es preciso
}

// Liberar un bloque
void block_free(uint32_t block_num) {
    block_mark_free(block_num);
}

// Leer un bloque del disco
int block_read(uint32_t block_num, void *buffer) {
    if (block_num >= block_ctx.total_blocks) {
        fprintf(stderr, "Error: Número de bloque %u fuera de rango (máx %zu)\n", 
                block_num, block_ctx.total_blocks - 1);
        return -1;
    }

    // Primero intentar leer el archivo PNG
    if (block_load_from_png(block_num, buffer) == 0) {
        return 0;  // Éxito al leer el PNG
    }

    // Si falla, intentar leer el archivo binario
    char *filename = block_get_filename(block_num);
    FILE *f = fopen(filename, "rb");
    if (!f) {
        perror("Error al abrir archivo de bloque para lectura");
        return -1;
    }

    size_t read = fread(buffer, 1, BLOCK_SIZE, f);
    fclose(f);

    if (read != BLOCK_SIZE) {
        fprintf(stderr, "Error: No se pudo leer el bloque completo (%zu/%d bytes)\n", 
                read, BLOCK_SIZE);
        return -1;
    }

    return 0;
}

// Escribir un bloque en el disco
int block_write(uint32_t block_num, const void *data) {
    if (block_num >= block_ctx.total_blocks) {
        fprintf(stderr, "Error: Número de bloque %u fuera de rango (máx %zu)\n", 
                block_num, block_ctx.total_blocks);
        return -1;
    }

    // Guardar en formato binario
    char *filename = block_get_filename(block_num);
    FILE *f = fopen(filename, "wb");
    if (!f) {
        perror("Error al abrir archivo de bloque para escritura");
        return -1;
    }

    size_t written = fwrite(data, 1, BLOCK_SIZE, f);
    fclose(f);

    if (written != BLOCK_SIZE) {
        fprintf(stderr, "Error: No se pudo escribir el bloque completo (%zu/%d bytes)\n", 
                written, BLOCK_SIZE);
        return -1;
    }

    // Guardar también como PNG
    if (block_save_as_png(block_num, data) != 0) {
        fprintf(stderr, "Advertencia: No se pudo guardar el bloque como PNG\n");
        // No es un error fatal, continuamos
    }

    // Actualizar el mapa de bits si es un bloque nuevo
    if (block_is_free(block_num)) {
        block_mark_used(block_num);
        block_ctx.superblock.free_blocks--;
        block_ctx.free_blocks--;
        time(&block_ctx.superblock.modified);
    }

    return 0;
}
