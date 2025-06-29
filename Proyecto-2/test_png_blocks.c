#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <assert.h>
#include "../include/block.h"

#define TEST_DIR "test_blocks"

void print_hex(const uint8_t *data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        printf("%02x ", data[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n");
}

void cleanup() {
    // Esta función ya no eliminará automáticamente el directorio
    printf("Nota: El directorio de prueba no se eliminará automáticamente.\n");
    printf("Puedes encontrarlo en: %s\n", TEST_DIR);
    printf("Para limpiar manualmente, ejecuta: rm -rf %s\n", TEST_DIR);
}

int main() {
    // Initialize block system
    const char *test_dir = TEST_DIR;
    const size_t num_blocks = 2;  // Reduced for testing
    
    printf("Creating test directory: %s\n", test_dir);
    
    // Clean up any previous test directory
    cleanup();
    
    // Create test directory
    printf("Creating test directory: %s\n", test_dir);
    if (mkdir(test_dir, 0755) != 0) {
        perror("Failed to create test directory");
        return 1;
    }
    
    printf("Initializing block system...\n");
    if (block_init(test_dir, num_blocks) != 0) {
        fprintf(stderr, "Failed to initialize block system\n");
        return 1;
    }
    
    // Use the actual BLOCK_SIZE for testing
    const size_t test_size = BLOCK_SIZE;  // Use the actual block size
    printf("Using test size: %zu bytes (BLOCK_SIZE=%d)\n", test_size, BLOCK_SIZE);
    
    uint8_t *test_data = malloc(test_size);
    uint8_t *read_data = malloc(test_size);
    
    if (!test_data || !read_data) {
        perror("Failed to allocate memory for test buffers");
        if (test_data) free(test_data);
        if (read_data) free(read_data);
        cleanup();
        return 1;
    }
    
    if (!test_data || !read_data) {
        perror("Memory allocation failed");
        free(test_data);
        free(read_data);
        cleanup();
        return 1;
    }
    
    // Fill test data with a simple pattern
    printf("Filling test data buffer (%zu bytes)...\n", test_size);
    for (size_t i = 0; i < test_size; i++) {
        test_data[i] = (uint8_t)(i % 256);
    }
    printf("Test data buffer filled. First byte: 0x%02x, last byte: 0x%02x\n", 
           test_data[0], test_data[test_size-1]);
    
    printf("Test data prepared (first 32 bytes):\n");
    print_hex(test_data, test_size > 32 ? 32 : test_size);
    
    printf("Testing block write/read...\n");
    
    // Test writing and reading blocks
    for (uint32_t i = 0; i < num_blocks; i++) {
        printf("Testing block %u...\n", i);
        
        printf("\n--- Testing block %u ---\n", i);
        
        // Write block
        printf("Writing block %u...\n", i);
        printf("Test data starts with: %02x %02x %02x...\n", 
               test_data[0], test_data[1], test_data[2]);
        
        int write_result = block_write(i, test_data);
        printf("block_write returned: %d\n", write_result);
        if (write_result != 0) {
            fprintf(stderr, "Failed to write block %u (error: %d)\n", i, write_result);
            free(test_data);
            free(read_data);
            block_cleanup();
            cleanup();
            return 1;
        }
        printf("Block %u written successfully.\n", i);
        
        // Clear read buffer
        memset(read_data, 0, test_size);
        
        // Read block
        printf("Reading block %u...\n", i);
        
        // Initialize read buffer with a known pattern
        memset(read_data, 0xAA, test_size);
        
        int read_result = block_read(i, read_data);
        printf("block_read returned: %d\n", read_result);
        
        if (read_result != 0) {
            fprintf(stderr, "Failed to read block %u (error: %d)\n", i, read_result);
            free(test_data);
            free(read_data);
            block_cleanup();
            cleanup();
            return 1;
        }
        
        printf("Read data (first 32 bytes):\n");
        print_hex(read_data, test_size > 32 ? 32 : test_size);
        
        // Verify data
        if (memcmp(test_data, read_data, test_size) != 0) {
            fprintf(stderr, "Data mismatch in block %u\n", i);
            printf("Expected (first 32 bytes):\n");
            print_hex(test_data, test_size > 32 ? 32 : test_size);
            printf("Got (first 32 bytes):\n");
            print_hex(read_data, test_size > 32 ? 32 : test_size);
            free(test_data);
            free(read_data);
            block_cleanup();
            cleanup();
            return 1;
        }
        
        printf("Block %u verified successfully!\n", i);
    }
    
    printf("\nAll tests passed! Check the PNG files in %s\n", test_dir);
    
    // List the generated files
    printf("\nGenerated files in %s:\n", test_dir);
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "ls -la %s", test_dir);
    system(cmd);
    
    // Clean up
    free(test_data);
    free(read_data);
    block_cleanup();
    cleanup();
    return 0;
}
