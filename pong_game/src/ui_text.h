#ifndef UI_TEXT_H
#define UI_TEXT_H

/* Modulo GENERICO de texto -- reemplaza a menu_text.h/.c.
 *
 * A diferencia de menu_text.c (que tenia las 3 frases del menu, sus
 * colores y sus posiciones todas fijas/hardcodeadas adentro), este modulo
 * NO sabe nada de que pantalla lo esta llamando: solo sabe dibujar UNA
 * linea de texto centrada horizontalmente, en el color y la fila Y que le
 * digas. Todo el contenido especifico de cada interfaz (que dice, en que
 * color, en que posicion) vive en main.c, en una funcion draw_state_*()
 * por estado -- asi se puede editar cada pantalla sin tocar este archivo.
 *
 * Usa draw_string() de font.h (fuente compacta 8x8, ya confirmada limpia
 * en tu hardware). Cada caracter mide 8px de ancho, asi que frases largas
 * (como "SELECCIONA EL MAESTRO CON SW15") entran en una sola linea de
 * 640px sin necesidad de partirlas.
 */
void ui_draw_text_centered(int y, const char *text, unsigned char color);

#endif