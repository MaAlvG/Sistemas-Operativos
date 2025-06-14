#include "../include/block.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <time.h>

#define TEST_BLOCKS 10  // Número de bloques para pruebas

// Declaraciones adelantadas de funciones de bloques
int block_init(const char *base_path, size_t total_blocks);
void block_cleanup(void);
uint32_t block_allocate(void);
void block_free(uint32_t block_num);
int block_is_free(uint32_t block_num);
int block_read(uint32_t block_num, void *buffer);
int block_write(uint32_t block_num, const void *data);

// Implementaciones simuladas para funciones PNG
int bwfs_write_png_block(const char *filename, const uint8_t *data, int width, int height) {
    (void)width; (void)height;  // No utilizados
    FILE *f = fopen(filename, "wb");
    if (!f) return -1;
    size_t written = fwrite(data, 1, BLOCK_SIZE, f);
    fclose(f);
    return (written == BLOCK_SIZE) ? 0 : -1;
}

int bwfs_read_png_block(const char *filename, uint8_t *data, int *width, int *height) {
    *width = 1000;  // Valores simulados para pruebas
    *height = 1000;
    
    FILE *f = fopen(filename, "rb");
    if (!f) return -1;
    size_t read = fread(data, 1, BLOCK_SIZE, f);
    fclose(f);
    return (read == BLOCK_SIZE) ? 0 : -1;
}

// Función auxiliar para verificar datos del bloque
static int verify_block_data(const uint8_t *data, uint8_t value) {
    for (size_t i = 0; i < BLOCK_SIZE; i++) {
        if (data[i] != value) {
            fprintf(stderr, "Discrepancia en datos en desplazamiento %zu: se esperaba 0x%02x, se obtuvo 0x%02x\n", 
                    i, value, data[i]);
            return -1;
        }
    }
    return 0;
}

int test_block_allocation() {
    printf("=== Iniciando Prueba de Asignación de Bloques ===\n");
    
    // Primero crear el directorio de prueba
    printf("Creando directorio de prueba...\n");
    system("rm -rf test_blocks");
    if (mkdir("test_blocks", 0755) < 0) {
        perror("Error al crear directorio de prueba");
        exit(EXIT_FAILURE);
    }
    
    // Inicializar el gestor de bloques
    printf("Inicializando gestor de bloques...\n");
    if (block_init("test_blocks", TEST_BLOCKS) != 0) {
        fprintf(stderr, "Error al inicializar el gestor de bloques\n");
        exit(EXIT_FAILURE);
    }
    
    // Probar asignación de bloques
    printf("\n--- Probando asignación de bloques ---\n");
    
    // Asignar un bloque
    uint32_t block1 = block_allocate();
    if (block1 == 0) {
        fprintf(stderr, "Error al asignar bloque\n");
        block_cleanup();
        exit(EXIT_FAILURE);
    }
    printf("Bloque asignado: %u\n", block1);
    
    // Verificar que el bloque esté marcado como ocupado
    if (block_is_free(block1)) {
        fprintf(stderr, "Error: El bloque recién asignado %u está marcado como libre\n", block1);
        block_cleanup();
        exit(EXIT_FAILURE);
    }
    
    // Intentar asignar otro bloque
    uint32_t block2 = block_allocate();
    if (block2 == 0) {
        fprintf(stderr, "Error al asignar segundo bloque\n");
        block_cleanup();
        exit(EXIT_FAILURE);
    }
    printf("Segundo bloque asignado: %u\n", block2);
    
    // Liberar el primer bloque
    printf("Liberando bloque %u\n", block1);
    block_free(block1);
    
    // Verificar que el bloque esté marcado como libre
    if (!block_is_free(block1)) {
        fprintf(stderr, "Error: El bloque liberado %u aún está marcado como ocupado\n", block1);
        block_cleanup();
        exit(EXIT_FAILURE);
    }
    
    // Asignar un nuevo bloque (debería reutilizar el bloque liberado)
    uint32_t block3 = block_allocate();
    if (block3 == 0) {
        fprintf(stderr, "Error al asignar tercer bloque\n");
        block_cleanup();
        exit(EXIT_FAILURE);
    }
    
    // Debería reutilizar el primer bloque
    if (block3 != block1) {
        fprintf(stderr, "Error: Se esperaba reutilizar el bloque %u, se obtuvo %u\n", block1, block3);
        block_cleanup();
        exit(EXIT_FAILURE);
    }
    printf("Bloque %u reutilizado exitosamente\n", block1);
    
    // Probar escritura de bloques
    printf("\n--- Probando escritura de bloques ---\n");
    uint8_t write_data[BLOCK_SIZE];
    memset(write_data, 0xAA, sizeof(write_data));
    
    // Escribir en el bloque
    printf("Escribiendo datos de prueba en el bloque %u...\n", block1);
    if (block_write(block1, write_data) != 0) {
        fprintf(stderr, "Error al escribir en el bloque\n");
        block_cleanup();
        exit(EXIT_FAILURE);
    }
    printf("Escritura exitosa de %d bytes en el bloque %u\n", BLOCK_SIZE, block1);
    
    // Probar lectura de bloques
    printf("\n--- Probando lectura de bloques ---\n");
    uint8_t read_data[BLOCK_SIZE];
    
    printf("Leyendo del bloque %u...\n", block1);
    if (block_read(block1, read_data) != 0) {
        fprintf(stderr, "Error al leer el bloque\n");
        block_cleanup();
        exit(EXIT_FAILURE);
    }
    
    // Verificar datos
    if (verify_block_data(read_data, 0xAA) != 0) {
        fprintf(stderr, "Error en la verificación de datos\n");
        block_cleanup();
        exit(EXIT_FAILURE);
    }
    printf("Datos del bloque %u leídos y verificados exitosamente\n", block1);
    
    // Probar escritura con datos diferentes
    printf("\n--- Probando escritura con datos diferentes ---\n");
    memset(write_data, 0x55, sizeof(write_data));
    
    printf("Escribiendo nuevos datos en el bloque %u...\n", block2);
    if (block_write(block2, write_data) != 0) {
        fprintf(stderr, "Error al escribir en el bloque\n");
        block_cleanup();
        exit(EXIT_FAILURE);
    }
    
    // Leer y verificar los nuevos datos
    printf("Leyendo del bloque %u...\n", block2);
    if (block_read(block2, read_data) != 0) {
        fprintf(stderr, "Error al leer el bloque\n");
        block_cleanup();
        exit(EXIT_FAILURE);
    }
    
    // Verificar los nuevos datos
    if (verify_block_data(read_data, 0x55) != 0) {
        fprintf(stderr, "Error en la verificación de datos después de actualizar\n");
        block_cleanup();
        exit(EXIT_FAILURE);
    }
    printf("Datos actualizados y verificados exitosamente en el bloque %u\n", block2);
    
    // Probar asignación de todos los bloques
    printf("\n--- Probando asignación de todos los bloques ---\n");
    uint32_t blocks[TEST_BLOCKS] = {0};
    int allocated_blocks = 0;
    
    // Asignar todos los bloques restantes
    for (int i = 0; i < TEST_BLOCKS - 2; i++) {  // -2 porque ya asignamos 2 bloques
        blocks[i] = block_allocate();
        if (blocks[i] == 0) {
            fprintf(stderr, "Error al asignar bloque %d\n", i);
            break;
        }
        printf("Bloque asignado: %u\n", blocks[i]);
        allocated_blocks++;
    }
    
    // La siguiente asignación debería fallar (sin bloques libres)
    if (block_allocate() != 0) {
        fprintf(stderr, "Error: Se esperaba que fallara la asignación (sin bloques libres)\n");
        block_cleanup();
        exit(EXIT_FAILURE);
    }
    printf("Correctamente falló al asignar bloque cuando no hay bloques libres\n");
    
    // Liberar algunos bloques
    printf("Liberando algunos bloques...\n");
    for (int i = 0; i < allocated_blocks; i += 2) {
        printf("Liberando bloque %u\n", blocks[i]);
        block_free(blocks[i]);
        blocks[i] = 0;  // Marcar como liberado
    }
    
    // Ahora deberíamos poder asignar bloques nuevamente, hasta el número que liberamos
    printf("Asignando bloques después de liberar algunos...\n");
    int blocks_allocated = 0;
    for (int i = 0; i < allocated_blocks; i++) {
        if (blocks[i] == 0) {  // Encontrar un espacio libre
            blocks[i] = block_allocate();
            if (blocks[i] == 0) {
                fprintf(stderr, "Error al asignar bloque después de liberar\n");
                block_cleanup();
                exit(EXIT_FAILURE);
            }
            printf("Bloque asignado: %u\n", blocks[i]);
            blocks_allocated++;
        }
    }
    
    // La siguiente asignación debería fallar nuevamente
    if (block_allocate() != 0) {
        fprintf(stderr, "Error: Se esperaba que fallara la asignación después de usar todos los bloques\n");
        block_cleanup();
        exit(EXIT_FAILURE);
    }
    printf("Correctamente falló al asignar bloque cuando no hay bloques libres\n");
    
    // Liberar todos los bloques
    printf("Liberando todos los bloques...\n");
    for (int i = 0; i < TEST_BLOCKS; i++) {
        if (blocks[i] != 0) {
            printf("Liberando bloque %u\n", blocks[i]);
            block_free(blocks[i]);
            blocks[i] = 0;
        }
    }
    
    // Limpieza
    printf("\n--- Limpiando ---\n");
    block_cleanup();
    
    printf("\n¡Todas las pruebas de gestión de bloques se completaron exitosamente!\n");
    return 0;
}

int main() {
    printf("=== Iniciando Pruebas de Gestión de Bloques BWFS ===\n\n");
    test_block_allocation();
    
    printf("\n=== ¡Todas las pruebas se completaron exitosamente! ===\n");
    return 0;
}
