//! @title pixel_mux
//! @author Jesús Huertas
//! @date 23/05/2026
//!
//! Multiplexor de píxel para el controlador VGA. Recibe un nibble de
//! 4 bits leído desde la VRAM y lo convierte en salida RGB de 12 bits
//! compatible con el conector VGA de la Nexys A7.
//!
//! Los 4 bits se interpretan como índice de paleta fija de 16 colores.
//! Cada canal RGB replica los 4 bits en su salida de 4 bits, produciendo
//! una paleta estándar de 16 colores RGBI. El índice 0 siempre produce
//! negro independientemente de video_on.
//!
//! La salida RGB se registra en flip-flops para mejorar el timing
//! y reducir glitching. Esto introduce una latencia adicional de
//! un ciclo que vga_controller compensa adelantando la dirección
//! de lectura de la VRAM en dos ciclos.

module pixel_mux (
    input  wire        clk,         //! Reloj de 25 MHz dominio VGA
    input  wire [3:0]  pixel_nibble, //! Nibble leído de la VRAM: índice de paleta 0-15
    input  wire        video_on,    //! Activo cuando el píxel está en la zona visible
    output reg  [3:0]  vga_r,       //! Canal rojo, 4 bits, para conector VGA Nexys A7
    output reg  [3:0]  vga_g,       //! Canal verde, 4 bits, para conector VGA Nexys A7
    output reg  [3:0]  vga_b        //! Canal azul, 4 bits, para conector VGA Nexys A7
);

    reg [3:0] r_out, g_out, b_out;

    always @(*) begin
        case (pixel_nibble)
            4'h0: begin r_out = 4'h0; g_out = 4'h0; b_out = 4'h0; end // negro
            4'h1: begin r_out = 4'h0; g_out = 4'h0; b_out = 4'hA; end // azul oscuro
            4'h2: begin r_out = 4'h0; g_out = 4'hA; b_out = 4'h0; end // verde oscuro
            4'h3: begin r_out = 4'h0; g_out = 4'hA; b_out = 4'hA; end // cian oscuro
            4'h4: begin r_out = 4'hA; g_out = 4'h0; b_out = 4'h0; end // rojo oscuro
            4'h5: begin r_out = 4'hA; g_out = 4'h0; b_out = 4'hA; end // magenta oscuro
            4'h6: begin r_out = 4'hA; g_out = 4'h5; b_out = 4'h0; end // naranja oscuro
            4'h7: begin r_out = 4'hA; g_out = 4'hA; b_out = 4'hA; end // gris claro
            4'h8: begin r_out = 4'h5; g_out = 4'h5; b_out = 4'h5; end // gris oscuro
            4'h9: begin r_out = 4'h5; g_out = 4'h5; b_out = 4'hF; end // azul claro
            4'hA: begin r_out = 4'h5; g_out = 4'hF; b_out = 4'h5; end // verde claro
            4'hB: begin r_out = 4'h5; g_out = 4'hF; b_out = 4'hF; end // cian claro
            4'hC: begin r_out = 4'hF; g_out = 4'h5; b_out = 4'h5; end // rojo claro
            4'hD: begin r_out = 4'hF; g_out = 4'h5; b_out = 4'hF; end // magenta claro
            4'hE: begin r_out = 4'hF; g_out = 4'hF; b_out = 4'h5; end // amarillo
            4'hF: begin r_out = 4'hF; g_out = 4'hF; b_out = 4'hF; end // blanco
        endcase
    end

    always @(posedge clk) begin
        if (video_on) begin
            vga_r <= r_out;
            vga_g <= g_out;
            vga_b <= b_out;
        end else begin
            vga_r <= 4'h0;
            vga_g <= 4'h0;
            vga_b <= 4'h0;
        end
    end

endmodule