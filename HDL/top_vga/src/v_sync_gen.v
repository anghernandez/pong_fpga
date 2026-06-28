
//! @title v_sync_gen
//! @author Jesús Huertas
//! @date 20/04/2026
//!
//! Generador de sincronización vertical para el estándar VGA
//! 640x480 @ 60Hz. Implementa un contador de 10 bits que avanza
//! únicamente cuando recibe el pulso h_end proveniente de h_sync_gen,
//! acoplándose al barrido horizontal sin conocer hcount internamente.
//!
//! Parámetros del estándar VGA 640x480 @ 60Hz:
//! - Zona visible:   480 líneas (0   a 479)
//! - Front porch:     10 líneas (480 a 489)
//! - Pulso vsync:      2 líneas (490 a 491)
//! - Back porch:      33 líneas (492 a 524)
//! - Total:          525 líneas

module v_sync_gen (
    input  wire        clk,    //! Reloj de 25 MHz dominio VGA
    input  wire        rst,    //! Reset síncrono activo alto
    input  wire        h_end,  //! Pulso de fin de línea desde h_sync_gen
    output reg  [9:0]  vcount, //! Posición vertical actual, rango 0-524
    output wire        vsync,  //! Sincronización vertical, activo bajo
    output wire        vblank  //! Activo cuando vcount >= 480 (zona no visible)
);

    localparam V_VISIBLE    = 480; //! Líneas visibles por frame
    localparam V_FP         = 10;  //! Front porch en líneas
    localparam V_SYNC_START = V_VISIBLE + V_FP;  //! Inicio del pulso vsync = 490
    localparam V_SYNC_END   = V_SYNC_START + 2;  //! Fin del pulso vsync = 492
    localparam V_TOTAL      = 525; //! Total de líneas por frame

    always @(posedge clk) begin
        if (rst) begin
            vcount <= 10'd0;
        end else if (h_end) begin
            if (vcount == V_TOTAL - 1)
                vcount <= 10'd0;
            else
                vcount <= vcount + 10'd1;
        end
    end

    assign vsync  = ~((vcount >= V_SYNC_START) && (vcount < V_SYNC_END));
    assign vblank =   (vcount >= V_VISIBLE);
endmodule