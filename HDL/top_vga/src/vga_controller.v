//! @title vga_controller
//! @author Jesús Huertas
//! @date 23/05/2026
//!
//! Controlador VGA completo para resolución 640x480 @ 60Hz con soporte
//! de color de 4 bits por pixel (16 colores de paleta fija).
//!
//! La VRAM almacena palabras de 32 bits donde cada palabra empaqueta
//! 8 píxeles. vram_addr apunta con 2 ciclos de adelanto para compensar
//! la latencia de la BRAM. nibble_sel se calcula desde hcount/vcount
//! actuales (sin adelanto) para quedar alineado con vram_data_in que
//! llega 1 ciclo después de la dirección.
//!
//! Pipeline de 3 etapas:
//!   Ciclo 0 (hcount=X): vram_addr presenta dirección para pixel X+2
//!                       nibble_sel presenta nibble para pixel X (hcount actual)
//!   Ciclo 1 (hcount=X+1): BRAM responde con la palabra del pixel X-1
//!                          nibble_sel presenta nibble para pixel X+1
//!   Ciclo 2 (hcount=X+2): pixel_mux registra RGB del pixel X → salida válida

module vga_controller (
    input  wire        clk,
    input  wire        rst,
    input  wire [31:0] vram_data_in,
    output wire [15:0] vram_addr,
    output wire        hsync,
    output wire        vsync,
    output wire [3:0]  vga_r,
    output wire [3:0]  vga_g,
    output wire [3:0]  vga_b
);

    localparam H_VISIBLE = 640;
    localparam H_TOTAL   = 800;
    localparam V_VISIBLE = 480;
    localparam V_TOTAL   = 525;

    wire [9:0] hcount, vcount;
    wire       hblank, vblank, h_end;

    h_sync_gen u_h_sync (
        .clk(clk), .rst(rst),
        .hcount(hcount), .hsync(hsync), .hblank(hblank), .h_end(h_end));

    v_sync_gen u_v_sync (
        .clk(clk), .rst(rst),
        .h_end(h_end), .vcount(vcount), .vsync(vsync), .vblank(vblank));

    wire video_on = ~hblank && ~vblank;

    reg video_on_r;
    always @(posedge clk)
        video_on_r <= video_on;

    // Dirección adelantada 2 ciclos para compensar latencia de BRAM + pixel_mux
    reg [9:0] next_hcount, next_vcount;
    always @(*) begin
        if (hcount >= H_TOTAL - 2) begin
            next_hcount = hcount - (H_TOTAL - 2);
            next_vcount = (vcount == V_TOTAL - 1) ? 10'd0 : vcount + 10'd1;
        end else begin
            next_hcount = hcount + 10'd2;
            next_vcount = vcount;
        end
    end

    wire [18:0] pixel_index_ahead = (next_hcount < H_VISIBLE && next_vcount < V_VISIBLE) ?
                                     next_vcount * H_VISIBLE + next_hcount : 19'd0;

    assign vram_addr = pixel_index_ahead[18:3]; //! Dirección de palabra adelantada 2 ciclos

    // nibble_sel usa hcount/vcount actuales (1 ciclo de adelanto respecto al RGB final)
    // Esto alinea el nibble con el dato que la BRAM devuelve 1 ciclo después de la dirección
    wire [18:0] pixel_index_cur = (hcount < H_VISIBLE && vcount < V_VISIBLE) ?
                                   vcount * H_VISIBLE + hcount : 19'd0;

    reg [2:0] nibble_sel_r;
    always @(posedge clk) nibble_sel_r <= pixel_index_cur[2:0];
    wire [2:0] nibble_sel = nibble_sel_r; //! Nibble del pixel actual

    wire [3:0] pixel_nibble = vram_data_in[nibble_sel * 4 +: 4];

    pixel_mux u_pixel_mux (
        .clk(clk), .pixel_nibble(pixel_nibble), .video_on(video_on_r),
        .vga_r(vga_r), .vga_g(vga_g), .vga_b(vga_b));

endmodule