//! @title axi_vram_ctrl
//! @author Jesús Huertas
//! @date 07/06/2026
//!
//! Controlador AXI4-Lite para escritura de VRAM desde MicroBlaze.
//! Implementa los cinco canales AXI4-Lite completos (AW, W, B, AR, R)
//! para que el SmartConnect pueda enrutar la interfaz correctamente.
//!
//! Los canales AW, W y B realizan la escritura real a la VRAM.
//! Los canales AR y R responden SLVERR a cualquier intento de lectura
//! ya que la VRAM es write-only desde el bus AXI.
//!
//! Compile/sim:
//! iverilog -g2012 -o sim axi_vram_ctrl.v tb.v && vvp sim

module axi_vram_ctrl #(
    parameter ADDR_WIDTH = 16
)(
    input  wire        S_AXI_ACLK,
    input  wire        S_AXI_ARESETN,

    // Canal AW — dirección de escritura
    input  wire [31:0] S_AXI_AWADDR,
    input  wire        S_AXI_AWVALID,
    output reg         S_AXI_AWREADY,

    // Canal W — dato de escritura
    input  wire [31:0] S_AXI_WDATA,
    input  wire [3:0]  S_AXI_WSTRB,
    input  wire        S_AXI_WVALID,
    output reg         S_AXI_WREADY,

    // Canal B — respuesta de escritura
    output reg  [1:0]  S_AXI_BRESP,
    output reg         S_AXI_BVALID,
    input  wire        S_AXI_BREADY,

    // Canal AR — dirección de lectura (responde SLVERR)
    input  wire [31:0] S_AXI_ARADDR,
    input  wire        S_AXI_ARVALID,
    output reg         S_AXI_ARREADY,

    // Canal R — dato de lectura (responde SLVERR)
    output reg  [31:0] S_AXI_RDATA,
    output reg  [1:0]  S_AXI_RRESP,
    output reg         S_AXI_RVALID,
    input  wire        S_AXI_RREADY,

    // Interfaz hacia VRAM
    output reg  [ADDR_WIDTH-1:0] VRAM_ADDR,
    output reg  [31:0]           VRAM_DATA,
    output reg  [3:0]            VRAM_WE
);

    localparam IDLE = 1'b0;
    localparam RESP = 1'b1;

    reg state;
    reg aw_received, w_received;
    reg [31:0] aw_addr_reg;
    reg [31:0] w_data_reg;
    reg [3:0]  w_strb_reg;

    wire both_ready = (aw_received || (S_AXI_AWVALID && S_AXI_AWREADY)) &&
                      (w_received  || (S_AXI_WVALID  && S_AXI_WREADY));

    // Registro de estado
    always @(posedge S_AXI_ACLK) begin
        if (!S_AXI_ARESETN)
            state <= IDLE;
        else case (state)
            IDLE: if (both_ready)   state <= RESP;
            RESP: if (S_AXI_BREADY) state <= IDLE;
            default: state <= IDLE;
        endcase
    end

    // Canal AW
    always @(posedge S_AXI_ACLK) begin
        if (!S_AXI_ARESETN) begin
            aw_received   <= 1'b0;
            aw_addr_reg   <= 32'h0;
            S_AXI_AWREADY <= 1'b0;
        end else begin
            if (state == IDLE && !aw_received)
                S_AXI_AWREADY <= 1'b1;

            if (state == IDLE && S_AXI_AWVALID && S_AXI_AWREADY && !aw_received) begin
                aw_addr_reg   <= S_AXI_AWADDR;
                aw_received   <= 1'b1;
                S_AXI_AWREADY <= 1'b0;
            end

            if (both_ready && state == IDLE) begin
                aw_received   <= 1'b0;
                S_AXI_AWREADY <= 1'b0;
            end

            if (state == RESP && S_AXI_BREADY) begin
                aw_received   <= 1'b0;
                S_AXI_AWREADY <= 1'b1;
            end
        end
    end

    // Canal W
    always @(posedge S_AXI_ACLK) begin
        if (!S_AXI_ARESETN) begin
            w_received   <= 1'b0;
            w_data_reg   <= 32'h0;
            w_strb_reg   <= 4'h0;
            S_AXI_WREADY <= 1'b0;
        end else begin
            if (state == IDLE && !w_received)
                S_AXI_WREADY <= 1'b1;

            if (state == IDLE && S_AXI_WVALID && S_AXI_WREADY && !w_received) begin
                w_data_reg   <= S_AXI_WDATA;
                w_strb_reg   <= S_AXI_WSTRB;
                w_received   <= 1'b1;
                S_AXI_WREADY <= 1'b0;
            end

            if (both_ready && state == IDLE) begin
                w_received   <= 1'b0;
                S_AXI_WREADY <= 1'b0;
            end

            if (state == RESP && S_AXI_BREADY) begin
                w_received   <= 1'b0;
                S_AXI_WREADY <= 1'b1;
            end
        end
    end

    // Canal B
    always @(posedge S_AXI_ACLK) begin
        if (!S_AXI_ARESETN) begin
            S_AXI_BVALID <= 1'b0;
            S_AXI_BRESP  <= 2'b00;
        end else begin
            if (state == IDLE && both_ready) begin
                S_AXI_BVALID <= 1'b1;
                S_AXI_BRESP  <= 2'b00;
            end else if (S_AXI_BREADY) begin
                S_AXI_BVALID <= 1'b0;
            end
        end
    end

    // Canal AR y R — la VRAM es write-only desde AXI
    // Cualquier lectura recibe SLVERR (2'b10) con dato 0xDEADBEEF
    always @(posedge S_AXI_ACLK) begin
        if (!S_AXI_ARESETN) begin
            S_AXI_ARREADY <= 1'b0;
            S_AXI_RVALID  <= 1'b0;
            S_AXI_RDATA   <= 32'h0;
            S_AXI_RRESP   <= 2'b00;
        end else begin
            if (S_AXI_ARVALID && !S_AXI_ARREADY) begin
                S_AXI_ARREADY <= 1'b1;
                S_AXI_RDATA   <= 32'hDEADBEEF;
                S_AXI_RRESP   <= 2'b10;
                S_AXI_RVALID  <= 1'b1;
            end else begin
                S_AXI_ARREADY <= 1'b0;
            end
            if (S_AXI_RVALID && S_AXI_RREADY)
                S_AXI_RVALID <= 1'b0;
        end
    end

    // Escritura VRAM
    always @(posedge S_AXI_ACLK) begin
        if (!S_AXI_ARESETN) begin
            VRAM_ADDR <= {ADDR_WIDTH{1'b0}};
            VRAM_DATA <= 32'h0;
            VRAM_WE   <= 4'h0;
        end else begin
            VRAM_WE <= 4'h0;
            if (state == IDLE && both_ready) begin
                VRAM_ADDR <= aw_received
                             ? aw_addr_reg[ADDR_WIDTH+1:2]
                             : S_AXI_AWADDR[ADDR_WIDTH+1:2];
                VRAM_DATA <= w_received ? w_data_reg : S_AXI_WDATA;
                VRAM_WE   <= w_received ? w_strb_reg : S_AXI_WSTRB;
            end
        end
    end

endmodule