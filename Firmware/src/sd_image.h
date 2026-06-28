#ifndef SD_IMAGE_H
#define SD_IMAGE_H

#include "xil_io.h"

/* Direcciones de memoria */
#define DDR2_IMAGE_BASE  0x80100000   /* Donde se carga la imagen en DDR2 (1 MB offset para no conflictuar con caché VGA) */
#define IMAGE_SIZE       (480 * 480 / 2)  /* 480×480 × 4 bits = 460800 bytes */

/* Inicializar MicroSD y cargar imagen */
int sd_image_init(void);

/* Leer imagen desde SD y copiar a DDR2 */
int sd_image_load(const char *filename);

/* Obtener puntero a imagen cargada en DDR2 */
unsigned char* sd_image_get_buffer(void);

#endif
