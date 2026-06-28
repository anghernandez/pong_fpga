//! @title h_sync_gen
//! @author Jesús Huertas
//! @date 20/04/2026
//!
//! Generador de sincronización horizontal para el estándar VGA
//! 640x480 @ 60Hz. Implementa un contador de 10 bits que opera
//! en el dominio de 25MHz y recorre los 800 ciclos que conforman
//! una línea horizontal completa, incluyendo la zona visible,
//! los porches y el pulso de sincronización.
//!
//! Parámetros del estándar VGA 640x480 @ 60Hz:
//! - Zona visible:   640 ciclos (0   a 639)
//! - Front porch:     16 ciclos (640 a 655)
//! - Pulso hsync:     96 ciclos (656 a 751)
//! - Back porch:      48 ciclos (752 a 799)
//! - Total:          800 ciclos

module h_sync_gen (
    input  wire        clk,    //! Reloj de 25 MHz dominio VGA
    input  wire        rst,    //! Reset síncrono activo alto
    output reg  [9:0]  hcount, //! Posición horizontal actual, rango 0-799
    output wire        hsync,  //! Sincronización horizontal, activo bajo
    output wire        hblank, //! Activo cuando hcount >= 640 (zona no visible)
    output wire        h_end   //! Pulso de fin de línea, activo cuando hcount == 799
);

    localparam H_VISIBLE    = 640; //! Píxeles visibles por línea
    localparam H_FP         = 16;  //! Front porch en ciclos
    localparam H_SYNC_START = H_VISIBLE + H_FP; //! Inicio del pulso hsync = 656
    localparam H_SYNC_END   = H_SYNC_START + 96; //! Fin del pulso hsync = 752
    localparam H_TOTAL      = 800; //! Total de ciclos por línea

    always @(posedge clk) begin
        if (rst) begin
            hcount <= 10'd0;
        end else begin
            if (hcount == H_TOTAL - 1)
                hcount <= 10'd0;
            else
                hcount <= hcount + 10'd1;
        end
    end

    assign hsync  = ~((hcount >= H_SYNC_START) && (hcount < H_SYNC_END));
    assign hblank =   (hcount >= H_VISIBLE);
    assign h_end  =   (hcount == H_TOTAL - 1);

endmodule