# BWFS - Sistema de Archivos en Blanco y Negro

BWFS es un sistema de archivos que utiliza imágenes en blanco y negro para almacenar datos. Cada bloque de datos se representa como una imagen de 1000x1000 píxeles, donde cada píxel puede ser blanco (1) o negro (0).

## Requisitos

- Sistema operativo Linux
- FUSE (Filesystem in Userspace)
- Bibliotecas de desarrollo de FUSE
- Compilador C (gcc o clang)
- Bibliotecas de desarrollo de PNG (libpng-dev)

## Instalación

1. Clona el repositorio:
   ```bash
   git clone <URL_DEL_REPOSITORIO>
   cd Proyecto-2
   ```

2. Compila el proyecto:
   ```bash
   make
   ```

3. Instala los binarios en el sistema (opcional):
   ```bash
   sudo make install
   ```

## Uso

### Crear un nuevo sistema de archivos

```bash
./bin/mkfs_bwfs /ruta/al/directorio
```

### Montar el sistema de archivos

```bash
./mount.bwfs /ruta/al/directorio /punto/de/montaje
```

### Desmontar el sistema de archivos

```bash
fusermount -u /punto/de/montaje
```

### Verificar la integridad del sistema de archivos

```bash
./bin/fsck_bwfs /ruta/al/directorio
```

## Estructura del Proyecto

- `src/`: Código fuente
  - `bwfs_impl.c`: Implementación principal de FUSE
  - `mkfs_bwfs.c`: Herramienta para crear sistemas de archivos
  - `fsck_bwfs.c`: Herramienta para verificar la integridad
- `include/`: Archivos de cabecera
  - `bwfs.h`: Definiciones de estructuras y constantes
  - `block.h`: Manejo de bloques de almacenamiento
- `bin/`: Binarios compilados

## Pruebas

Para ejecutar las pruebas básicas:

```bash
make test-fs      # Crear sistema de archivos de prueba
make test-mount   # Montar sistema de archivos de prueba
make test-fsck    # Verificar integridad
make test-umount  # Desmontar sistema de archivos
make clean-test   # Limpiar archivos de prueba
```

## Licencia

Este proyecto está bajo la licencia MIT. Ver el archivo LICENSE para más detalles.
