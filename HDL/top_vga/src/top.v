//! @title top
//! @author Jesús Huertas
//! @date 07/06/2026
//!
//! Módulo top del subsistema VGA para el proyecto Pong.
//! Integra la VRAM, el controlador AXI y el controlador VGA
//! en un solo módulo instanciable desde el block design de Vivado.
//!
//! El reloj de 25 MHz para el dominio VGA se recibe como puerto
//! clk_25 desde el Clocking Wizard del block design principal.
//! Esto evita instanciar IPs de Vivado dentro del IP empaquetado.
//!
//! Puerto clk      : 100 MHz — escrituras AXI a la VRAM
//! Puerto clk_25   : 25 MHz  — dominio VGA (desde clk_out3 del block design)
//! Puerto S_AXI_*  : bus AXI4-Lite desde MicroBlaze via SmartConnect
//! Puerto VGA_*    : salidas directas al conector VGA de la Nexys A7

module top (
    //! Reloj de 100 MHz de la Nexys A7
    input  wire        clk,
    //! Reloj de 25 MHz para el dominio VGA, viene del Clocking Wizard del block design
    input  wire        clk_25,
    //! Reset activo alto desde el Processor System Reset
    input  wire        reset,

    // ── Bus AXI4-Lite — conectado desde el SmartConnect ──────────────
    input  wire        S_AXI_ACLK,
    input  wire        S_AXI_ARESETN,
    input  wire [31:0] S_AXI_AWADDR,
    input  wire        S_AXI_AWVALID,
    output wire        S_AXI_AWREADY,
    input  wire [31:0] S_AXI_WDATA,
    input  wire [3:0]  S_AXI_WSTRB,
    input  wire        S_AXI_WVALID,
    output wire        S_AXI_WREADY,
    output wire [1:0]  S_AXI_BRESP,
    output wire        S_AXI_BVALID,
    input  wire        S_AXI_BREADY,
    input  wire [31:0] S_AXI_ARADDR,
    input  wire        S_AXI_ARVALID,
    output wire        S_AXI_ARREADY,
    output wire [31:0] S_AXI_RDATA,
    output wire [1:0]  S_AXI_RRESP,
    output wire        S_AXI_RVALID,
    input  wire        S_AXI_RREADY,

    // ── Salidas VGA ───────────────────────────────────────────────────
    output wire        VGA_HS,
    output wire        VGA_VS,
    output wire [3:0]  VGA_R,
    output wire [3:0]  VGA_G,
    output wire [3:0]  VGA_B
);

    // ── Reset del dominio VGA ─────────────────────────────────────────
    // Activo alto mientras el reset del sistema esté activo
    wire rst_vga = reset;

    // ── Interconnect VRAM ─────────────────────────────────────────────
    wire [15:0] vram_addr_a;
    wire [31:0] vram_data_a;
    wire [3:0]  vram_we_a;
    wire [15:0] vram_addr_b;
    wire [31:0] vram_data_b;

    // ── Instancia VRAM ────────────────────────────────────────────────
    vram u_vram (
        .clk_a     (S_AXI_ACLK),
        .we_a      (vram_we_a),
        .addr_a    (vram_addr_a),
        .data_in_a (vram_data_a),
        .clk_b     (clk_25),
        .addr_b    (vram_addr_b),
        .data_out_b(vram_data_b)
    );

    // ── Instancia AXI VRAM Controller ────────────────────────────────
    axi_vram_ctrl #(.ADDR_WIDTH(16)) u_axi_vram (
        .S_AXI_ACLK    (S_AXI_ACLK),
        .S_AXI_ARESETN (S_AXI_ARESETN),
        .S_AXI_AWADDR  (S_AXI_AWADDR),
        .S_AXI_AWVALID (S_AXI_AWVALID),
        .S_AXI_AWREADY (S_AXI_AWREADY),
        .S_AXI_WDATA   (S_AXI_WDATA),
        .S_AXI_WSTRB   (S_AXI_WSTRB),
        .S_AXI_WVALID  (S_AXI_WVALID),
        .S_AXI_WREADY  (S_AXI_WREADY),
        .S_AXI_BRESP   (S_AXI_BRESP),
        .S_AXI_BVALID  (S_AXI_BVALID),
        .S_AXI_BREADY  (S_AXI_BREADY),
        .S_AXI_ARADDR  (S_AXI_ARADDR),
        .S_AXI_ARVALID (S_AXI_ARVALID),
        .S_AXI_ARREADY (S_AXI_ARREADY),
        .S_AXI_RDATA   (S_AXI_RDATA),
        .S_AXI_RRESP   (S_AXI_RRESP),
        .S_AXI_RVALID  (S_AXI_RVALID),
        .S_AXI_RREADY  (S_AXI_RREADY),
        .VRAM_ADDR     (vram_addr_a),
        .VRAM_DATA     (vram_data_a),
        .VRAM_WE       (vram_we_a)
    );

    // ── Instancia VGA Controller ──────────────────────────────────────
    vga_controller u_vga (
        .clk         (clk_25),
        .rst         (rst_vga),
        .vram_data_in(vram_data_b),
        .vram_addr   (vram_addr_b),
        .hsync       (VGA_HS),
        .vsync       (VGA_VS),
        .vga_r       (VGA_R),
        .vga_g       (VGA_G),
        .vga_b       (VGA_B)
    );

endmodule