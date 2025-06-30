# BWFS - Black and White File System

BWFS es un sistema de archivos en espacio de usuario (FUSE) que utiliza imágenes en blanco y negro como medio de almacenamiento.

## Componentes

- `mkfs.bwfs`: Formatea un directorio para ser utilizado por BWFS, creando las estructuras iniciales del sistema de archivos en imágenes PNG.
- `mount.bwfs`: Monta el sistema de archivos BWFS en un punto de montaje del sistema operativo.
- `fsck.bwfs`: Realiza una comprobación de consistencia en un sistema de archivos BWFS.

## Compilación

Para compilar el proyecto, ejecuta el siguiente comando:

```bash
sh build.sh
```

## Uso

1.  **Crear el sistema de archivos:**

    ```bash
    ./build/mkfs.bwfs ./data
    ```

2.  **Montar el sistema de archivos:**

    ```bash
    ./build/mount.bwfs ./data ./mount
    ```

3.  **Verificar el punto de montaje:**

    ```bash
    ls -la ./mount
    ```

4.  **Desmontar:**

    ```bash
    fusermount -u ./mount
    ```
