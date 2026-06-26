#include "input.h"
#include "xil_io.h"

#define DEBOUNCE_COUNT 2  /* Número de lecturas idénticas para confirmar (reducido) */

/* Estados para debounce de botones */
static unsigned int btn_previous = 0;
static int btn_stable_count = 0;
static unsigned int btn_candidate = 0;

/* Estados para debounce de switches */
static unsigned int sw_previous = 0;
static int sw_stable_count = 0;
static unsigned int sw_candidate = 0;

void input_init(void) {
    /* Configurar GPIO como entrada */
    Xil_Out32(GPIO_BUTTONS_BASE + GPIO_TRI_REG, 0xFFFFFFFF);
    Xil_Out32(GPIO_SWITCHES_BASE + GPIO_TRI_REG, 0xFFFFFFFF);
}

/* Debounce para botones - retorna flancos ascendentes */
unsigned int input_read_buttons(void) {
    unsigned int raw = Xil_In32(GPIO_BUTTONS_BASE + GPIO_DATA_REG) & 0xF;

    /* Si cambió la lectura raw */
    if (raw != btn_candidate) {
        btn_candidate = raw;
        btn_stable_count = 1;
        return 0;  /* No confirmado aún */
    }

    /* Misma lectura, incrementar contador */
    btn_stable_count++;
    if (btn_stable_count >= DEBOUNCE_COUNT) {
        /* Estado estabilizado, detectar flanco ascendente */
        unsigned int pressed = raw & ~btn_previous;
        btn_previous = raw;
        btn_stable_count = 0;
        return pressed;
    }

    return 0;
}

/* Debounce para switches - retorna flancos ascendentes */
unsigned int input_read_switches(void) {
    unsigned int raw = Xil_In32(GPIO_SWITCHES_BASE + GPIO_DATA_REG) & 0xFFFF;

    /* Si cambió la lectura raw */
    if (raw != sw_candidate) {
        sw_candidate = raw;
        sw_stable_count = 1;
        return 0;  /* No confirmado aún */
    }

    /* Misma lectura, incrementar contador */
    sw_stable_count++;
    if (sw_stable_count >= DEBOUNCE_COUNT) {
        /* Estado estabilizado, detectar flanco ascendente */
        unsigned int pressed = raw & ~sw_previous;
        sw_previous = raw;
        sw_stable_count = 0;
        return pressed;
    }

    return 0;
}