#!/bin/bash

# Script para compilar el proyecto BWFS

# --- Configuración ---

# Directorios
BUILD_DIR="build"
SRC_DIR="src"
INCLUDE_DIR="include"

# Compilador y flags
CC="gcc"
# Usamos pkg-config para obtener los flags de FUSE y libpng
# Asegúrate de tener instalados: libfuse-dev, libpng-dev
# En Debian/Ubuntu: sudo apt-get install libfuse-dev libpng-dev pkg-config
CFLAGS="-Wall -I${INCLUDE_DIR} $(pkg-config --cflags fuse3) $(pkg-config --cflags libpng)"
LDFLAGS="$(pkg-config --libs fuse3) $(pkg-config --libs libpng)"

# Binarios a generar
TARGETS="mkfs.bwfs mount.bwfs fsck.bwfs"

# --- Compilación ---

# Crear directorio de compilación
mkdir -p ${BUILD_DIR}

# --- Compilación ---

# Crear directorio de compilación
mkdir -p ${BUILD_DIR}

# Archivos fuente comunes (por ahora solo bwfs_image.c)
COMMON_SOURCES="${SRC_DIR}/bwfs_image.c"

# Compilar mkfs.bwfs
echo "Compilando mkfs.bwfs..."
${CC} ${CFLAGS} -o ${BUILD_DIR}/mkfs.bwfs ${SRC_DIR}/mkfs.bwfs.c ${COMMON_SOURCES} ${LDFLAGS}
if [ $? -ne 0 ]; then echo "Error al compilar mkfs.bwfs"; exit 1; fi

# Compilar mount.bwfs
echo "Compilando mount.bwfs..."
${CC} ${CFLAGS} -o ${BUILD_DIR}/mount.bwfs ${SRC_DIR}/mount.bwfs.c ${COMMON_SOURCES} ${LDFLAGS}
if [ $? -ne 0 ]; then echo "Error al compilar mount.bwfs"; exit 1; fi

# Compilar fsck.bwfs
echo "Compilando fsck.bwfs..."
${CC} ${CFLAGS} -o ${BUILD_DIR}/fsck.bwfs ${SRC_DIR}/fsck.bwfs.c ${COMMON_SOURCES} ${LDFLAGS}
if [ $? -ne 0 ]; then echo "Error al compilar fsck.bwfs"; exit 1; fi


echo "
Compilación finalizada. Los binarios se encuentran en el directorio '${BUILD_DIR}'."
