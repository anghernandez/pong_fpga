#ifndef FONT_H
#define FONT_H

#include "xil_types.h"

#define CHAR_WIDTH  8
#define CHAR_HEIGHT 8

/* Renderiza un carácter (A-Z, a-z, 0-9, espacio) en (x, y) con color */
void draw_char(int x, int y, char c, unsigned char color);

/* Renderiza una cadena de texto a partir de (x, y) */
void draw_string(int x, int y, const char *str, unsigned char color);

/* ── Renderizador ROBUSTO (solo palabras uniformes) ─────────────────────────
 * Dibuja cada pixel del glifo como un bloque de 8px de ancho con draw_rect,
 * de modo que NUNCA se escribe una palabra con nibbles mezclados (que es lo
 * que este IP no maneja bien). El caracter sale magnificado: cada pixel de la
 * fuente mide 8px de ancho * vscale de alto. Un caracter completo mide
 * 8*8 = 64px de ancho * 8*vscale de alto.
 */
void draw_char_big(int x, int y, char c, unsigned char color, int vscale);
void draw_string_big(int x, int y, const char *str, unsigned char color, int vscale);

/* ── Experimento: draw_char COMPACTO (8px) con corrimiento de nibbles ────────
 * Igual que draw_char pero rota la posicion del nibble por `rot` (circular,
 * 0..7). rot=0 == draw_char original. Sirve para encontrar el corrimiento que
 * corrige el desfase de posicion observado en el render compacto.
 */
void draw_char_rot(int x, int y, char c, unsigned char color, int rot);

/* Como draw_char_rot pero con opcion de espejo horizontal (mirror=0/1).
 * Sirve para descartar que el texto salga espejado (no detectable con '0').
 */
void draw_char_xform(int x, int y, char c, unsigned char color, int rot, int mirror);

#endif