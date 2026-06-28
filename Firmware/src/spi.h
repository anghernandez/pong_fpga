#ifndef SPI_H
#define SPI_H

#include "xil_io.h"

/* ── Periférico GPIO_3 → PMOD JA (bidireccional) ───────────────────────── */
#define SPI_DATA  0x40030000
#define SPI_TRI   0x40030004

/* Pines (bit positions en GPIO_3) */
#define PIN_SCK   (1u << 0)   /* JA1 - C17 */
#define PIN_MOSI  (1u << 1)   /* JA2 - D18 */
#define PIN_MISO  (1u << 2)   /* JA3 - E18 */
#define PIN_SS    (1u << 3)   /* JA4 - G17 */

/* TRI: 0 = salida, 1 = entrada */
#define TRI_MASTER  (PIN_MISO)                    /* master lee MISO; conduce SCK, MOSI, SS */
#define TRI_SLAVE   (PIN_SCK | PIN_MOSI | PIN_SS) /* esclavo lee SCK, MOSI, SS; conduce MISO */

/*
 * Conexión física PMOD JA ↔ PMOD JA:
 *   JA1 (SCK)  maestro → esclavo
 *   JA2 (MOSI) maestro → esclavo JA3
 *   JA3 (MISO) maestro ← esclavo JA2
 *   JA4 (SS)   maestro → esclavo
 */

/* Configura GPIO_3 como maestro SPI (SCK/MOSI/SS como salidas, MISO entrada) */
void spi_init_master(void);

/* Configura GPIO_3 como esclavo SPI (SCK/MOSI/SS como entradas, MISO salida) */
void spi_init_slave(void);

/* Transfiere un byte como maestro; retorna el byte recibido del esclavo */
unsigned char spi_master_transfer(unsigned char tx);

/*
 * Transfiere un byte como esclavo; escribe el byte recibido en *rx_out.
 * Retorna 0 si OK, -1 si timeout esperando flanco SCK.
 */
int spi_slave_transfer(unsigned char tx, unsigned char *rx_out);

#endif /* SPI_H */
