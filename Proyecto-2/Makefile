CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=31
LIBS = -lfuse3 -lpng -lz -lm
SRCDIR = src
BINDIR = bin

# Archivos fuente
BINARIES = bwfs_impl mkfs_bwfs fsck_bwfs
TARGETS = $(addprefix $(BINDIR)/, $(BINARIES))

# Objetivo por defecto
all: directories $(TARGETS)

# Crear directorios necesarios
directories:
	@mkdir -p $(BINDIR)

# Regla genérica para compilar binarios
$(BINDIR)/%: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -Iinclude -o $@ $< $(LIBS)

# Instalar binarios en el sistema
install: all
	sudo cp $(BINDIR)/bwfs_impl /usr/local/bin/
	sudo cp $(BINDIR)/mkfs_bwfs /usr/local/bin/
	sudo cp $(BINDIR)/fsck_bwfs /usr/local/bin/
	sudo cp mount.bwfs /usr/local/bin/
	sudo chmod +x /usr/local/bin/mount.bwfs

# Desinstalar binarios del sistema
uninstall:
	sudo rm -f /usr/local/bin/bwfs_impl
	sudo rm -f /usr/local/bin/mkfs_bwfs
	sudo rm -f /usr/local/bin/fsck_bwfs
	sudo rm -f /usr/local/bin/mount.bwfs

# Limpiar archivos compilados
clean:
	rm -rf $(BINDIR)

# Crear un sistema de archivos de prueba
test-fs: $(BINDIR)/mkfs_bwfs
	mkdir -p test_bwfs
	$(BINDIR)/mkfs_bwfs test_bwfs
	@echo "Sistema de archivos de prueba creado en test_bwfs/"

# Montar sistema de archivos de prueba
test-mount: test-fs
	mkdir -p test_mount
	./mount.bwfs test_bwfs test_mount
	@echo "Sistema de archivos montado en test_mount/"

# Desmontar sistema de archivos de prueba
test-umount:
	fusermount -u test_mount 2>/dev/null || true
	rmdir test_mount 2>/dev/null || true

# Verificar sistema de archivos de prueba
test-fsck: $(BINDIR)/fsck_bwfs
	$(BINDIR)/fsck_bwfs test_bwfs

# Limpiar archivos de prueba
clean-test: test-umount
	rm -rf test_bwfs test_mount

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

.PHONY: all directories install uninstall clean test-fs test-mount test-umount test-fsck clean-test help
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
