#include "spi.h"

static void spi_delay(volatile int n) { while (n--); }


//Configura la FPGA como maestro. Deja SCK, MOSI y SS como salidas, y MISO como entrada.
void spi_init_master(void) {
    Xil_Out32(SPI_TRI, TRI_MASTER);
    Xil_Out32(SPI_DATA, PIN_SS);   /* SS inactivo (alto) al arrancar */
}
//Configura la FPGA como esclavo. Deja SCK, MOSI y SS como entradas, y MISO como salida.
void spi_init_slave(void) {
    Xil_Out32(SPI_TRI, TRI_SLAVE);
    Xil_Out32(SPI_DATA, 0u);
}

//El maestro envía un byte bit por bit, desde el bit más significativo. Para cada bit

/*
coloca dato en MOSI
sube SCK
lee MISO
baja SCK
*/
unsigned char spi_master_transfer(unsigned char tx) {
    unsigned char rx = 0;
    unsigned int mosi_bit;
    int bit;

    for (bit = 7; bit >= 0; bit--) {
        mosi_bit = (tx & (1u << bit)) ? PIN_MOSI : 0u;
        Xil_Out32(SPI_DATA, mosi_bit);
        spi_delay(50);
        Xil_Out32(SPI_DATA, mosi_bit | PIN_SCK);
        spi_delay(50);
        if (Xil_In32(SPI_DATA) & PIN_MISO)
            rx |= (1u << bit);
        Xil_Out32(SPI_DATA, mosi_bit);
        spi_delay(50);
    }
    return rx;
}

//El esclavo espera los pulsos de reloj del maestro. En cada pulso:
/*
coloca su dato en MISO
espera SCK alto
lee MOSI
espera SCK bajo
*/
int spi_slave_transfer(unsigned char tx, unsigned char *rx_out) {
    unsigned char rx = 0;
    int bit, t;

    for (bit = 7; bit >= 0; bit--) {
        Xil_Out32(SPI_DATA, (tx & (1u << bit)) ? PIN_MISO : 0u);

        t = 500000;
        while (!(Xil_In32(SPI_DATA) & PIN_SCK) && --t);
        if (!t) return -1;

        if (Xil_In32(SPI_DATA) & PIN_MOSI)
            rx |= (1u << bit);

        t = 500000;
        while ((Xil_In32(SPI_DATA) & PIN_SCK) && --t);
        if (!t) return -1;
    }
    *rx_out = rx;
    return 0;
}