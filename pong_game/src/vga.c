#include "vga.h"
#include <stdint.h>

/* Caché en DDR (0x80000000) para evitar leer del hardware VGA
 * DDR tiene 128 MB disponible, usamos los primeros 153.6 KB para caché de pantalla
 * Esto evita cuelgues en read_word y permite RMW rápido
 * CRÍTICO: usar uint32_t explícitamente (no unsigned int) para garantizar 32 bits
 */
static uint32_t *vga_cache = (uint32_t *)0x81000000; // antes 0x80000000

//Primer cambio VGA para poder pintar bien a ball
/*void write_word(int word_addr, unsigned int value) {
    if (word_addr < TOTAL_WORDS) {
        vga_cache[word_addr] = (uint32_t)value;
        Xil_Out32(VGA_BASE + (word_addr << 2), (uint32_t)value);
    }
}*/


void write_word(int word_addr, unsigned int value) {
    if (word_addr >= 0 && word_addr < TOTAL_WORDS) {
        vga_cache[word_addr] = (uint32_t)value;
        Xil_Out32(VGA_BASE + (word_addr << 2), (uint32_t)value);
    }
}

unsigned int read_word(int word_addr) {
    if (word_addr < TOTAL_WORDS) {
        return (unsigned int)vga_cache[word_addr];
    }
    return 0;
}

// ─── draw_pixel ───────────────────────────────────────────────────────────────
// modifica un solo nibble dentro del word sin tocar los otros 7 pixeles

void draw_pixel(int x, int y, unsigned char color) {
    if (x < 0 || x >= SCREEN_W || y < 0 || y >= SCREEN_H) return;

    int word_addr  = (y * WORDS_PER_LINE) + (x / 8);
    int nibble_pos = 7 - (x % 8);
    int shift      = nibble_pos * 4;

    // Escribe directamente sin leer primero
    unsigned int value = (color & 0xF) << shift;
    write_word(word_addr, value);
}

unsigned int color_to_word(unsigned char color) {
    unsigned int c = color & 0xF;
    return c | (c << 4) | (c << 8) | (c << 12) | 
           (c << 16) | (c << 20) | (c << 24) | (c << 28);
}

void clear_screen(unsigned char color) {
    unsigned int pattern = color_to_word(color);
    for (int i = 0; i < TOTAL_WORDS; i++) {
        write_word(i, pattern);
    }
}

void draw_rect(int x, int y, int w, int h, unsigned char color) {
    unsigned int pattern = color_to_word(color);
    
    // Calcular palabras por fila
    int start_word = (y * WORDS_PER_LINE) + (x / 8);
    int words_per_row = w / 8;
    
    // Dibujar fila por fila con words completas
    for(int row = 0; row < h; row++) {
        int word_addr = start_word + (row * WORDS_PER_LINE);
        for(int col = 0; col < words_per_row; col++) {
            write_word(word_addr + col, pattern);
        }
    }
}

// ─── draw_border ──────────────────────────────────────────────────────────────
// dibuja un borde de 1 pixel en los 4 lados de la pantalla


void draw_hline(int x, int y, int w, unsigned char color) {
    if (y < 0 || y >= SCREEN_H) return;
    if (x < 0) { w += x; x = 0; }
    if (x + w > SCREEN_W) w = SCREEN_W - x;
    if (w <= 0) return;

    unsigned int full_word = color_to_word(color);
    int x_end = x + w;

    // pixeles iniciales no alineados
    while (x < x_end && (x % 8) != 0) {
        draw_pixel(x, y, color);
        x++;
    }

    // words completas (camino rapido: 8 pixeles por escritura)
    while (x + 8 <= x_end) {
        write_word((y * WORDS_PER_LINE) + (x / 8), full_word);
        x += 8;
    }

    // pixeles finales no alineados
    while (x < x_end) {
        draw_pixel(x, y, color);
        x++;
    }
}

void draw_vline(int x, int y, int h, unsigned char color) {
    if (x < 0 || x >= SCREEN_W) return;
    if (y < 0) { h += y; y = 0; }
    if (y + h > SCREEN_H) h = SCREEN_H - y;
    if (h <= 0) return;

    // Ruta robusta para HW: dibujar la columna completa de 8 px alineada.
    // En este IP, la escritura por word es la operación más confiable.
    int x_aligned = x & ~7;
    draw_rect(x_aligned, y, 8, h, color);
}

void draw_border(unsigned char color) {
    draw_hline(0,            0,            SCREEN_W, color);  // top
    draw_hline(0,            SCREEN_H - 1, SCREEN_W, color);  // bottom
    draw_vline(0,            0,            SCREEN_H, color);  // left
    draw_vline(SCREEN_W - 1, 0,            SCREEN_H, color);  // right
}


void delay_ms(int ms) {
    for(volatile int i = 0; i < ms * 100000; i++);
}

/* Dibujar imagen 480×480 centrada (con fondo negro en los lados) */
void draw_image_centered(unsigned char *image_data) {
    int start_x = (SCREEN_W - 480) / 2;  /* (640 - 480) / 2 = 80 */
    int start_y = (SCREEN_H - 480) / 2;  /* (480 - 480) / 2 = 0 */

    /* Dibujar fondo negro en los lados */
    draw_rect(0, 0, start_x, SCREEN_H, COLOR_BLACK);  /* Lado izquierdo */
    draw_rect(start_x + 480, 0, start_x, SCREEN_H, COLOR_BLACK);  /* Lado derecho */

    /* Copiar imagen desde DDR2 a VGA */
    uint32_t src_offset = 0;
    for (int y = 0; y < 480; y++) {
        for (int x = 0; x < 480; x += 8) {  /* 8 píxeles = 1 word */
            int word_addr = ((start_y + y) * WORDS_PER_LINE) + ((start_x + x) / 8);
            uint32_t word_val = 0;

            /* Leer 8 píxeles (4 nibbles = 4 bytes) */
            for (int px = 0; px < 4; px++) {
            uint32_t byte_val = image_data[src_offset++];
            word_val |= (byte_val << ((3 - px) * 8));   /* invierte el orden de armado */
}

            write_word(word_addr, word_val);
        }
    }
}