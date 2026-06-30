#ifndef INPUT_H
#define INPUT_H

#define GPIO_BUTTONS_BASE   0x40010000
#define GPIO_SWITCHES_BASE  0x40020000
#define GPIO_DATA_REG       0x00
#define GPIO_TRI_REG        0x04

/* Inicializar GPIO para entrada */
void input_init(void);

/* Leer botones con debounce (retorna cambios detectados) */
unsigned int input_read_buttons(void);

/* Leer switches con debounce (retorna cambios detectados) */
unsigned int input_read_switches(void);

#endif