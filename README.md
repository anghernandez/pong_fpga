# pong_fpga

Sistema embebido multijugador sobre FPGA — EL3313 Taller de Diseño Digital, I Semestre 2026.

Videojuego Pong para dos jugadores implementado en C sobre un procesador MicroBlaze RISC-V
en modo bare-metal, corriendo en una Nexys A7. Incluye salida VGA, comunicación SPI entre
dos FPGAs, y carga de recursos gráficos desde microSD.

**Herramientas:** Vivado 2024.1 · Vitis 2024.1 · Nexys A7 (Artix-7 xc7a100t)

---

## Descripción del proyecto

El sistema implementa el videojuego Pong sobre un procesador MicroBlaze RISC-V corriendo en
modo bare-metal en una FPGA Nexys A7. El firmware escribe directamente a una VRAM ubicada en
BRAM, la cual es leída por un controlador VGA personalizado para generar la señal de video a
640×480 @ 60 Hz. Las imágenes del menú se cargan desde una microSD al inicio. La comunicación
multijugador se realiza mediante SPI bit-bang por GPIO sobre el conector PMOD JA, permitiendo
que dos FPGAs intercambien el estado del juego en tiempo real.

---

## Diagrama de bloques del sistema

El diagrama completo del sistema en Vivado IP Integrator puede verse en:
[`docs/block_diagram.pdf`](docs/block_diagram.pdf)

### Componentes principales del sistema

| Bloque | Función |
|---|---|
| MicroBlaze RISC-V | Procesador principal, ejecuta el firmware desde DDR2 |
| MIG 7 Series (DDR2) | Memoria externa — firmware, framebuffer, variables del juego |
| top_vga (IP custom) | Controlador VGA con VRAM AXI4, genera señal 640×480 @ 60 Hz |
| AXI GPIO ×4 | LEDs, botones, switches, SPI bit-bang (PMOD JA) |
| AXI Quad SPI | Comunicación con microSD |
| Clocking Wizard | Genera 100 MHz (AXI) y 25 MHz (dominio VGA) |

### Arquitectura del firmware

```
main.c
├── vga.c          → escritura al framebuffer en DDR2
├── font.c         → renderizado de texto y sprites
├── sd_image.c     → carga de imágenes desde microSD al inicio
├── input.c        → lectura de switches y botones
├── ui_text.c      → menús y pantallas de texto
├── pong_game.c    → lógica del juego single-player (IA)
├── pong_paddles.c → física de paletas y pelota
├── pong_mp.c      → modo multijugador vía SPI
└── spi.c          → driver SPI bit-bang sobre GPIO PMOD JA
```

---

## Reporte de utilización de recursos

Dispositivo: `xc7a100tcsg324-1` (Nexys A7-100T)

| Recurso | Usado | Disponible | Utilización |
|---|---|---|---|
| Slice LUTs | 16,829 | 63,400 | 26.5% |
| Slice Registers | 20,351 | 126,800 | 16.1% |
| Block RAM (tiles) | 68 | 135 | 50.4% |
| DSP Slices | 1 | 240 | 0.4% |
| IOBs | 108 | 210 | 51.4% |

> El uso elevado de BRAM (50.4%) corresponde principalmente a la VRAM del controlador VGA
> (64 de los 68 tiles utilizados).

---

## Reporte de timing

Reporte generado desde el diseño implementado (`impl_1`).

| Métrica | Valor |
|---|---|
| WNS (Worst Negative Slack) | +1.272 ns |
| TNS Failing Endpoints | 0 |
| WHS (Worst Hold Slack) | +0.052 ns |
| THS Failing Endpoints | 0 |
| Frecuencia principal (AXI/CPU) | 100 MHz |
| Frecuencia dominio VGA | 25 MHz |

**Todos los constraints de timing se cumplen.**

---

## Instrucciones de reproducibilidad

### Prerrequisitos

- Vitis 2024.1 (camino rápido) — o Vivado 2024.1 + Vitis 2024.1 (camino completo)
- Nexys A7-100T con cable USB-JTAG
- MicroSD con archivos `MENU.BIN` y `FIELD.BIN` (imágenes del menú)

### Paso 1 — Clonar el repositorio

```bash
git clone https://github.com/anghernandez/pong_fpga.git
cd pong_fpga
```

---

### Camino rápido — solo Vitis (recomendado)

El repositorio incluye el archivo `Hardware/xsaCHECK3.xsa` con el hardware ya compilado
y el bitstream embebido. No es necesario abrir Vivado.

**Paso 2 — Crear la plataforma en Vitis**

1. Abrir Vitis 2024.1 y crear un workspace nuevo
2. `File > New > Platform Project` → seleccionar `Hardware/xsaCHECK3.xsa`
3. Compilar la plataforma

**Paso 3 — Crear el application component**

1. `File > New > Application Project` → seleccionar la plataforma creada
2. Copiar todos los archivos de `Firmware/src/` al componente de la aplicación

**Paso 4 — Compilar y programar**

1. Click derecho en el application component → **Build**
2. Click en **Run** — Vitis programa el bitstream, descarga el ELF al MicroBlaze y ejecuta automáticamente

---

### Camino completo — reconstruir desde Vivado

Usar este camino solo si se necesita modificar el hardware (block design, IPs, constraints).

**Paso 2 — Recrear el proyecto Vivado**

Desde la Tcl Console de Vivado:

```tcl
cd <ruta_al_repositorio>
source create_project.tcl
```

Esto recrea el proyecto completo con el block design y todos los IPs.
Al abrir el proyecto por primera vez Vivado puede pedir actualizar los IPs — aceptar.

> **Nota:** el IP de VGA (`top_vga`) está incluido en `HDL/top_vga/` y el TCL
> lo registra automáticamente como IP repository al correr el script.

**Paso 3 — Generar el bitstream y exportar hardware**

1. En Vivado: `Flow > Generate Bitstream`
2. Al terminar: `File > Export > Export Hardware` → activar **Include Bitstream** → guardar como `.xsa`

**Paso 4 — Continuar desde el Paso 2 del camino rápido** usando el `.xsa` recién exportado.

---

## Documentación del IP de VGA

La documentación generada con TerosHDL de cada módulo del IP personalizado
está disponible en la carpeta [`docs/`](docs/):

- [`top.html`](docs/top.html) — módulo top del IP VGA
- [`axi_vram_ctrl.html`](docs/axi_vram_ctrl.html) — controlador AXI de la VRAM
- [`vram.html`](docs/vram.html) — memoria de video (BRAM)
- [`vga_controller.html`](docs/vga_controller.html) — generador de señal VGA
- [`h_sync_gen.html`](docs/h_sync_gen.html) — sincronización horizontal
- [`v_sync_gen.html`](docs/v_sync_gen.html) — sincronización vertical
- [`pixel_mux.html`](docs/pixel_mux.html) — multiplexor de píxeles RGB

---

## Controles y flujo del juego

Al encender la FPGA aparece el menú principal con dos opciones:

### Modo dos jugadores local (una sola FPGA)

Activar **SW0** desde el menú para iniciar.

| Jugador | Arriba | Abajo |
|---|---|---|
| Jugador 1 (paleta izquierda) | BTNL | BTND |
| Jugador 2 (paleta derecha) | BTNU | BTNR |

**BTNC** — salir al menú en cualquier momento.

### Modo multijugador SPI (dos FPGAs)

Activar **SW1** desde el menú para entrar al modo multijugador.

1. Cada FPGA selecciona su rol con **SW15**: `ON` = Maestro · `OFF` = Esclavo
2. **Primero** el Esclavo presiona **BTNC** para quedar en espera
3. **Luego** el Maestro presiona **BTNC** — se realiza el handshake SPI automáticamente
4. Si el handshake falla, ambas FPGAs regresan al menú

Durante el juego multijugador cada jugador controla su paleta con **BTNU** (arriba) y **BTND** (abajo). **BTNC** cancela y regresa al menú.

> **Conexión física PMOD JA:** directa pin a pin — JA1↔JA1, JA2↔JA2, JA3↔JA3, JA4↔JA4, GND↔GND.
> Esto funciona porque el GPIO es bidireccional: el software asigna la dirección de cada pin
> (entrada o salida) según el rol seleccionado con SW15, sin que haya conflicto en el cable.

---

## Curso

**EL3313 Taller de Diseño Digital** · I Semestre 2026  
Profesor: Luis G. León-Vega, Ph.D  
Instituto Tecnológico de Costa Rica
