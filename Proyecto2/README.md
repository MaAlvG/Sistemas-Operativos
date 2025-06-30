# Compilar mkfs.bwfs
gcc -Wall -Iinclude -o bin/mkfs.bwfs src/mkfs.bwfs.c src/bwfs_image.c $(pkg-config --cflags --libs fuse3) -lpng

# Compilar mount.bwfs
gcc -Wall -Iinclude -o bin/mount.bwfs src/mount.bwfs.c src/bwfs_image.c $(pkg-config --cflags --libs fuse3) -lpng

# Crear directorio para el sistema de archivos
mkdir -p bwfs

# Crear punto de montaje
mkdir -p bwfsmnt

# Crear el sistema de archivos
./bin/mkfs.bwfs bwfs

# Montar el sistema de archivos
./bin/mount.bwfs bwfs bwfsmnt


# Crear un archivo de prueba
echo "Contenido de prueba" > bwfsmnt/testfile.txt

# Leer el archivo
cat bwfsmnt/testfile.txt

# Listar directorio
ls -la bwfsmnt/

# Desmontar
fusermount -u bwfsmnt

# Montar el sistema de archivos
./bin/mount.bwfs bwfs bwfsmnt

# Listar directorio
ls -la bwfsmnt/

# Añadir contenido al archivo existente
echo "Segunda línea" >> bwfsmnt/testfile.txt

# Crear un directorio
mkdir bwfsmnt/testdir

# Crear un archivo en el directorio nuevo
echo "Archivo en directorio" > bwfsmnt/testdir/file_in_dir.txt

# Leer el archivo
cat bwfsmnt/testfile.txt

# Eliminar el archivo
rm bwfsmnt/testfile.txt

# Listar directorio
ls -la bwfsmnt/

# Intentar leer el archivo
cat bwfsmnt/testfile.txt

# ========== PRUEBAS DE TAMAÑO DE ARCHIVO ==========

# 50 KB
dd if=/dev/urandom of="bwfsmnt/small.bin" bs=50K count=1 status=none

# Probar con 100 KB (dentro del límite)
dd if=/dev/urandom of="bwfsmnt/small.bin" bs=100K count=1 status=none

# 122,070 bytes (máximo teórico)
dd if=/dev/urandom of="bwfsmnt/max_block.bin" bs=122070 count=1 status=none

# 1 MB
dd if=/dev/urandom of="bwfsmnt/medium.bin" bs=1M count=1 status=none

# ========== PRUEBAS DE TAMAÑO DE ARCHIVO ==========

# 50 KB
dd if=/dev/urandom of="bwfsmnt/small.bin" bs=50K count=1 status=none

# Probar con 100 KB (dentro del límite)
dd if=/dev/urandom of="bwfsmnt/small.bin" bs=100K count=1 status=none

# 122,070 bytes (máximo teórico)
dd if=/dev/urandom of="bwfsmnt/max_block.bin" bs=122070 count=1 status=none

# 1 MB
dd if=/dev/urandom of="bwfsmnt/medium.bin" bs=1M count=1 status=none

# ========== PRUEBAS DE TAMAÑO DE ARCHIVO ==========

# Desmontar
fusermount -u bwfsmnt
