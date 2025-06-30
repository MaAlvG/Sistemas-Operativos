#ifndef BWFS_IMAGE_H
#define BWFS_IMAGE_H

#include <png.h>
#include "bwfs.h"

// --- Funciones para la manipulación de imágenes ---

/**
 * @brief Crea una nueva imagen PNG en blanco y negro.
 * 
 * @param filepath Ruta donde se guardará la imagen.
 * @param width Ancho de la imagen.
 * @param height Alto de la imagen.
 * @return 0 si tiene éxito, -1 en caso de error.
 */
int create_bw_image(const char *filepath, int width, int height);

/**
 * @brief Escribe un buffer de datos en una imagen PNG.
 * 
 * Convierte los bytes del buffer en píxeles blanco y negro y los escribe
 * en la imagen especificada.
 * 
 * @param filepath Ruta de la imagen a modificar.
 * @param data Buffer con los datos a escribir.
 * @param size Tamaño del buffer de datos.
 * @return 0 si tiene éxito, -1 en caso de error.
 */
int write_data_to_image(const char *filepath, const unsigned char *data, size_t size);

/**
 * @brief Lee datos de una imagen PNG y los guarda en un buffer.
 * 
 * Convierte los píxeles de la imagen en bytes y los almacena en el buffer.
 * 
 * @param filepath Ruta de la imagen a leer.
 * @param data Buffer donde se guardarán los datos.
 * @param size Tamaño del buffer (debe ser suficiente para los datos de la imagen).
 * @return El número de bytes leídos, o -1 en caso de error.
 */
int read_data_from_image(const char *filepath, unsigned char *data, size_t size);


#endif // BWFS_IMAGE_H
