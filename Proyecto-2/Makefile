CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=31
LIBS = -lfuse3 -lpng -lz -lm
SRCDIR = src
BUILDDIR = build
BINDIR = bin
TESTDIR = tests

# Archivos fuente
MOUNT_SRC = $(SRCDIR)/bwfs.c
MKFS_SRC = $(SRCDIR)/bwfs.c
FSCK_SRC = $(SRCDIR)/bwfs.c

# Binarios objetivo
MOUNT_BIN = $(BINDIR)/mount.bwfs
MKFS_BIN = $(BINDIR)/mkfs.bwfs
FSCK_BIN = $(BINDIR)/fsck.bwfs
TEST_BLOCKS = $(BINDIR)/test_blocks

# Objetivos
all: directories $(MOUNT_BIN) $(MKFS_BIN) $(FSCK_BIN) $(TEST_BLOCKS)

directories:
	@mkdir -p $(BUILDDIR) $(BINDIR) $(SRCDIR)

$(MOUNT_BIN): $(MOUNT_SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

$(MKFS_BIN): $(MKFS_SRC)
	$(CC) $(CFLAGS) -DMKFS_BWFS -o $@ $< $(LIBS)

$(FSCK_BIN): $(FSCK_SRC)
	$(CC) $(CFLAGS) -DFSCK_BWFS -o $@ $< $(LIBS)

# Instalar binarios en el sistema
install: all
	sudo cp $(MOUNT_BIN) /usr/local/bin/
	sudo cp $(MKFS_BIN) /usr/local/bin/
	sudo cp $(FSCK_BIN) /usr/local/bin/
	sudo chmod +x /usr/local/bin/mount.bwfs
	sudo chmod +x /usr/local/bin/mkfs.bwfs
	sudo chmod +x /usr/local/bin/fsck.bwfs

# Desinstalar binarios del sistema
uninstall:
	sudo rm -f /usr/local/bin/mount.bwfs
	sudo rm -f /usr/local/bin/mkfs.bwfs
	sudo rm -f /usr/local/bin/fsck.bwfs

# Limpiar archivos compilados
# Test target
test-blocks: $(TEST_BLOCKS)
	@echo "Running block management tests..."
	@./$(TEST_BLOCKS)

# Clean target
clean:
	rm -rf $(BUILDDIR) $(BINDIR) test_blocks

# Test binary
$(TEST_BLOCKS): $(TESTDIR)/test_blocks.c $(SRCDIR)/block.c
	@mkdir -p $(BINDIR)
	$(CC) $(CFLAGS) -Iinclude -o $@ $^ $(LIBS)

# Crear un sistema de archivos de prueba
test-fs: $(MKFS_BIN)
	mkdir -p test_bwfs
	$(MKFS_BIN) test_bwfs 50
	@echo "Sistema de archivos de prueba creado en test_bwfs/"

# Montar sistema de archivos de prueba
test-mount: $(MOUNT_BIN) test-fs
	mkdir -p test_mount
	$(MOUNT_BIN) test_bwfs test_mount
	@echo "Sistema de archivos montado en test_mount/"

# Desmontar sistema de archivos de prueba
test-umount:
	fusermount -u test_mount 2>/dev/null || true
	rmdir test_mount 2>/dev/null || true

# Verificar sistema de archivos de prueba
test-fsck: $(FSCK_BIN)
	$(FSCK_BIN) test_bwfs

# Limpiar archivos de prueba
clean-test: test-umount
	rm -rf test_bwfs test_mount

# Ejecutar todas las pruebas
test: test-fs test-fsck
	@echo "Pruebas completadas exitosamente"

# Mostrar ayuda
help:
	@echo "Objetivos disponibles:"
	@echo "  all          - Compilar todos los binarios"
	@echo "  install      - Instalar binarios en /usr/local/bin"
	@echo "  uninstall    - Desinstalar binarios del sistema"
	@echo "  clean        - Limpiar archivos compilados"
	@echo "  test-fs      - Crear sistema de archivos de prueba"
	@echo "  test-mount   - Montar sistema de archivos de prueba"
	@echo "  test-umount  - Desmontar sistema de archivos de prueba"
	@echo "  test-fsck    - Verificar sistema de archivos de prueba"
	@echo "  clean-test   - Limpiar archivos de prueba"
	@echo "  test         - Ejecutar todas las pruebas"
	@echo "  help         - Mostrar esta ayuda"

# Dependencias
.PHONY: all directories install uninstall clean test-fs test-mount test-umount test-fsck clean-test test help

# Verificar dependencias del sistema
check-deps:
	@echo "Verificando dependencias..."
	@pkg-config --exists fuse3 || (echo "ERROR: libfuse3-dev no está instalado" && exit 1)
	@pkg-config --exists libpng || (echo "ERROR: libpng-dev no está instalado" && exit 1)
	@echo "Todas las dependencias están instaladas"

# Compilar con información de depuración
debug: CFLAGS += -g -DDEBUG
debug: all

# Compilar con optimizaciones
release: CFLAGS += -O2 -DNDEBUG
release: all
