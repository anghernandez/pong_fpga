#include "font.h"
#include "vga.h"
#include <stdint.h>

/* Bitmap font 8x8 - ALFABETO REAL
 * Bit order: cada byte = 8 pixeles, bit 7 = columna izquierda ... bit 0 = der.
 * Las letras ocupan las columnas centrales (bits 1-6, 6px de ancho) igual que
 * los digitos, con la fila 7 en blanco como separacion vertical.
 * El mapeo nibble->pixel calibrado vive en draw_char (nibble_pos = (5-col)&7).
 * Almacenado en DDR2 mediante sección .images
 *
 * NUEVO: se agregaron 4 glifos de puntuacion (indices 38-41): ¿ ? ( )
 * Diseñados con el mismo estilo/convencion que el resto de la tabla.
 */
static unsigned char font_bitmap[42][8] __attribute__((section(".images"))) = {
	/* A */ {0x3C, 0x42, 0x42, 0x7E, 0x42, 0x42, 0x42, 0x00},
	/* B */ {0x7C, 0x42, 0x42, 0x7C, 0x42, 0x42, 0x7C, 0x00},
	/* C */ {0x3C, 0x42, 0x40, 0x40, 0x40, 0x42, 0x3C, 0x00},
	/* D */ {0x7C, 0x42, 0x42, 0x42, 0x42, 0x42, 0x7C, 0x00},
	/* E */ {0x7E, 0x40, 0x40, 0x7C, 0x40, 0x40, 0x7E, 0x00},
	/* F */ {0x7E, 0x40, 0x40, 0x7C, 0x40, 0x40, 0x40, 0x00},
	/* G */ {0x3C, 0x42, 0x40, 0x4E, 0x42, 0x42, 0x3C, 0x00},
	/* H */ {0x42, 0x42, 0x42, 0x7E, 0x42, 0x42, 0x42, 0x00},
	/* I */ {0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x00},
	/* J */ {0x1E, 0x04, 0x04, 0x04, 0x44, 0x44, 0x38, 0x00},
	/* K */ {0x42, 0x44, 0x48, 0x70, 0x48, 0x44, 0x42, 0x00},
	/* L */ {0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x7E, 0x00},
	/* M */ {0x42, 0x66, 0x5A, 0x42, 0x42, 0x42, 0x42, 0x00},
	/* N */ {0x42, 0x62, 0x52, 0x4A, 0x46, 0x42, 0x42, 0x00},
	/* O */ {0x3C, 0x42, 0x42, 0x42, 0x42, 0x42, 0x3C, 0x00},
	/* P */ {0x7C, 0x42, 0x42, 0x7C, 0x40, 0x40, 0x40, 0x00},
	/* Q */ {0x3C, 0x42, 0x42, 0x42, 0x4A, 0x44, 0x3A, 0x00},
	/* R */ {0x7C, 0x42, 0x42, 0x7C, 0x48, 0x44, 0x42, 0x00},
	/* S */ {0x3C, 0x42, 0x40, 0x3C, 0x02, 0x42, 0x3C, 0x00},
	/* T */ {0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00},
	/* U */ {0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x3C, 0x00},
	/* V */ {0x42, 0x42, 0x42, 0x42, 0x42, 0x24, 0x18, 0x00},
	/* W */ {0x42, 0x42, 0x42, 0x42, 0x5A, 0x66, 0x42, 0x00},
	/* X */ {0x42, 0x24, 0x18, 0x18, 0x18, 0x24, 0x42, 0x00},
	/* Y */ {0x42, 0x24, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00},
	/* Z */ {0x7E, 0x04, 0x08, 0x10, 0x20, 0x40, 0x7E, 0x00},
	/* 0 */ {0x3C, 0x42, 0x42, 0x42, 0x42, 0x42, 0x3C, 0x00},  /* 0 clasico */
	/* 1 */ {0x08, 0x18, 0x08, 0x08, 0x08, 0x08, 0x3E, 0x00},  /* 1 clasico */
	/* 2 */ {0x3C, 0x42, 0x02, 0x0C, 0x10, 0x20, 0x7E, 0x00},  /* 2 clasico */
	/* 3 */ {0x3C, 0x42, 0x02, 0x1C, 0x02, 0x42, 0x3C, 0x00},  /* 3 clasico */
	/* 4 */ {0x04, 0x0C, 0x14, 0x24, 0x7E, 0x04, 0x04, 0x00},  /* 4 clasico */
	/* 5 */ {0x7E, 0x40, 0x40, 0x7C, 0x02, 0x42, 0x3C, 0x00},  /* 5 clasico */
	/* 6 */ {0x1C, 0x20, 0x40, 0x7C, 0x42, 0x42, 0x3C, 0x00},  /* 6 clasico */
	/* 7 */ {0x7E, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x00},  /* 7 clasico */
	/* 8 */ {0x3C, 0x42, 0x42, 0x3C, 0x42, 0x42, 0x3C, 0x00},  /* 8 clasico */
	/* 9 */ {0x3C, 0x42, 0x42, 0x3E, 0x02, 0x42, 0x3C, 0x00},  /* 9 clasico */
	/* space */ {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	/* . */ {0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00},
	/* ¿ (38) */ {0x18, 0x00, 0x18, 0x30, 0x40, 0x42, 0x3C, 0x00},
	/* ?  (39) */ {0x3C, 0x42, 0x02, 0x0C, 0x18, 0x00, 0x18, 0x00},
	/* (  (40) */ {0x0C, 0x10, 0x20, 0x20, 0x20, 0x10, 0x0C, 0x00},
	/* )  (41) */ {0x30, 0x08, 0x04, 0x04, 0x04, 0x08, 0x30, 0x00},
};

/* Mapear caracter ASCII a indice en font_bitmap */
static int char_to_index(char c) {
	if (c >= 'A' && c <= 'Z') return c - 'A';
	if (c >= 'a' && c <= 'z') return c - 'a';
	if (c >= '0' && c <= '9') return 26 + (c - '0');
	if (c == ' ') return 36;
	if (c == '.') return 37;
	if (c == (char)0xBF) return 38;  /* ¿  -- usar "\xBF" en el string, no el caracter literal (ver nota abajo) */
	if (c == '?') return 39;
	if (c == '(') return 40;
	if (c == ')') return 41;
	return -1;
}

/* Renderizar un caracter construyendo palabras completas (8 pixeles por escritura)
 * Sin shifts complicados - solo escribe palabras de 32 bits al hardware
 * Alinea x a multiple de 8 para escribir palabras completas sin residuo
 */
void draw_char(int x, int y, char c, unsigned char color) {
	int idx = char_to_index(c);
	if (idx < 0) return;

	unsigned char *glyph = font_bitmap[idx];
	uint32_t color_nibble = (color & 0xF);

	for (int row = 0; row < CHAR_HEIGHT; row++) {
		unsigned char byte = glyph[row];
		uint32_t word_value = 0;

		for (int col = 0; col < 8; col++) {
			if (byte & (1 << col)) {
				int nibble_pos = (5 - col) & 7;
				word_value |= (color_nibble << (nibble_pos * 4));
			}
		}

		int word_addr = (y + row) * WORDS_PER_LINE + (x / 8);
		write_word(word_addr, word_value);
	}
}

/* Renderizar una cadena de texto */
void draw_string(int x, int y, const char *str, unsigned char color) {
	int px = x;
	while (*str) {
		draw_char(px, y, *str, color);
		px += 8;
		str++;
	}
}

/* ── Renderizador ROBUSTO ───────────────────────────────────────────────────
 * Cada pixel encendido del glifo se dibuja con draw_rect (palabra UNIFORME),
 * que es el unico camino confiable en este IP. Nunca se escribe una palabra
 * con nibbles mezclados, asi que no depende del orden interno de los nibbles
 * (lo unico que importa es la direccion del word_addr, ya confirmada por el
 * test del '7' gigante).
 *
 * Orientacion: la fuente define bit7 = columna izquierda. Por eso la columna
 * de pantalla sx (0 = izquierda) se enciende segun el bit (7 - sx) del byte.
 * Si en el HW saliera espejado, basta poner FONT_MIRROR en 1.
 */
#define FONT_BLOCK_W 8   /* ancho de cada pixel del glifo, en px (1 palabra) */
#define FONT_MIRROR  0   /* 0 = orientacion natural; 1 = espejo horizontal */

void draw_char_big(int x, int y, char c, unsigned char color, int vscale) {
	int idx = char_to_index(c);
	if (idx < 0) return;
	if (vscale < 1) vscale = 1;

	x = (x / 8) * 8;  /* alinear a palabra: cada bloque cae en palabras enteras */
	unsigned char *glyph = font_bitmap[idx];

	for (int row = 0; row < CHAR_HEIGHT; row++) {
		unsigned char byte = glyph[row];
		for (int sx = 0; sx < 8; sx++) {
			int bit = FONT_MIRROR ? sx : (7 - sx);
			if (byte & (1 << bit)) {
				/* Bloque de 8px de ancho => draw_rect escribe palabras uniformes */
				draw_rect(x + sx * FONT_BLOCK_W, y + row * vscale,
				          FONT_BLOCK_W, vscale, color);
			}
		}
	}
}

void draw_string_big(int x, int y, const char *str, unsigned char color, int vscale) {
	int px = x;
	int advance = 8 * FONT_BLOCK_W + FONT_BLOCK_W;  /* ancho del glifo + 1 bloque de gap */
	while (*str) {
		draw_char_big(px, y, *str, color, vscale);
		px += advance;
		str++;
	}
}

/* ── Experimento: draw_char compacto con corrimiento de nibbles ──────────── */
void draw_char_rot(int x, int y, char c, unsigned char color, int rot) {
	int idx = char_to_index(c);
	if (idx < 0) return;

	x = (x / 8) * 8;
	unsigned char *glyph = font_bitmap[idx];
	uint32_t color_nibble = (color & 0xF);

	for (int row = 0; row < CHAR_HEIGHT; row++) {
		unsigned char byte = glyph[row];
		uint32_t word_value = 0;

		for (int col = 0; col < 8; col++) {
			if (byte & (1 << col)) {
				int nibble_pos = (col + rot) & 7;  /* corrimiento circular */
				word_value |= (color_nibble << (nibble_pos * 4));
			}
		}

		int word_addr = (y + row) * WORDS_PER_LINE + (x / 8);
		write_word(word_addr, word_value);
	}
}

/* ── Experimento: corrimiento + espejo opcional ─────────────────────────── */
void draw_char_xform(int x, int y, char c, unsigned char color, int rot, int mirror) {
	int idx = char_to_index(c);
	if (idx < 0) return;

	x = (x / 8) * 8;
	unsigned char *glyph = font_bitmap[idx];
	uint32_t color_nibble = (color & 0xF);

	for (int row = 0; row < CHAR_HEIGHT; row++) {
		unsigned char byte = glyph[row];
		uint32_t word_value = 0;

		for (int col = 0; col < 8; col++) {
			if (byte & (1 << col)) {
				int base = mirror ? (rot - col) : (rot + col);
				int nibble_pos = base & 7;  /* circular */
				word_value |= (color_nibble << (nibble_pos * 4));
			}
		}

		int word_addr = (y + row) * WORDS_PER_LINE + (x / 8);
		write_word(word_addr, word_value);
	}
}