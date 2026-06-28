//! @title vram
//! @author Jesús Huertas
//! @date 23/05/2026
//!
//! Memoria de video de doble puerto para el sistema VGA Pong.
//! Almacena el contenido de la pantalla como un mapa de nibbles de
//! 38,400 palabras de 32 bits, donde cada palabra empaqueta 8 píxeles
//! de 4 bits (índice de paleta). La resolución es 640x480 píxeles.
//!
//! Puerto A: escritura de 32 bits por MicroBlaze via AXI a 100 MHz.
//! Puerto B: lectura de 32 bits por vga_controller a 25 MHz.
//!
//! Cada palabra almacena los píxeles pixel_7 en bits 31:28 hasta
//! pixel_0 en bits 3:0, con pixel_0 en el nibble menos significativo.

// vram.v modificado - soporte para byte enables
module vram (
    input  wire        clk_a,
    input  wire [3:0]  we_a,        
    input  wire [15:0] addr_a,
    input  wire [31:0] data_in_a,
    input  wire        clk_b,
    input  wire [15:0] addr_b,
    output reg  [31:0] data_out_b
);

    localparam DEPTH = 640 * 480 / 8;
    reg [31:0] mem [0:DEPTH-1];

    integer i;
    initial begin
        for (i = 0; i < DEPTH; i = i + 1)
            mem[i] = 32'h0;
    end

    // Escritura con byte enables
    always @(posedge clk_a) begin
        if (we_a[0]) mem[addr_a][7:0]   <= data_in_a[7:0];
        if (we_a[1]) mem[addr_a][15:8]  <= data_in_a[15:8];
        if (we_a[2]) mem[addr_a][23:16] <= data_in_a[23:16];
        if (we_a[3]) mem[addr_a][31:24] <= data_in_a[31:24];
    end

    always @(posedge clk_b) begin
        data_out_b <= mem[addr_b];
    end

endmodule