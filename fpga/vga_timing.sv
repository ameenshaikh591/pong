`timescale 1ns/1ps

module vga_timing (
    input logic sys_clk_i,
    input logic reset_i,

    output logic [9:0] x,
    output logic [9:0] y,
    output logic hsync,
    output logic vsync,
    output logic video_on

);

    localparam H_DISPLAY = 640;
    localparam H_L_BORDER = 54;
    localparam H_R_BORDER = 10;
    localparam H_RETRACE = 96;
    localparam H_MAX = H_DISPLAY + H_L_BORDER + H_R_BORDER + H_RETRACE - 1;
    localparam START_H_RETRACE = H_DISPLAY + H_R_BORDER;
    localparam END_H_RETRACE = START_H_RETRACE + H_RETRACE - 1;

    localparam V_DISPLAY = 480; 
	localparam V_T_BORDER = 33; 
	localparam V_B_BORDER = 10; 
	localparam V_RETRACE = 2; 
	localparam V_MAX = V_DISPLAY + V_T_BORDER + V_B_BORDER + V_RETRACE - 1;
    localparam START_V_RETRACE = V_DISPLAY + V_B_BORDER;
	localparam END_V_RETRACE = V_DISPLAY + V_B_BORDER + V_RETRACE - 1;

    logic [1:0] clk_div_ctr;
    logic pixel_tick;

    /* 
    always_ff @(posedge sys_clk_i) begin
        if (clk_div_ctr == 3) begin
            clk_div_ctr <= 0;
        end else begin
            clk_div_ctr <= clk_div_ctr + 1;
        end
    end
    */

    assign pixel_tick = clk_div_ctr == 3;

    logic [9:0] hsync_ctr;
    logic [9:0] vsync_ctr;

    always_ff @(posedge sys_clk_i) begin
        if (reset_i) begin
            hsync_ctr <= 0;
            vsync_ctr <= 0;
            clk_div_ctr <= 0;
        end else begin

            if (clk_div_ctr == 3) begin
                clk_div_ctr <= 0;
            end else begin
                clk_div_ctr <= clk_div_ctr + 1;
            end

            if (pixel_tick) begin
                if (hsync_ctr == H_MAX) begin
                    hsync_ctr <= 0;
                    if (vsync_ctr == V_MAX) begin
                        vsync_ctr <= 0;
                    end else begin
                        vsync_ctr <= vsync_ctr + 1;
                    end
                end else begin
                    hsync_ctr <= hsync_ctr + 1;
                end
            end
        end

    end

    always_comb begin
        hsync = !(hsync_ctr >= START_H_RETRACE && hsync_ctr <= END_H_RETRACE);
        vsync = !(vsync_ctr >= START_V_RETRACE && vsync_ctr <= END_V_RETRACE); 
        video_on = hsync_ctr < H_DISPLAY && vsync_ctr < V_DISPLAY;
        x = hsync_ctr;
        y = vsync_ctr;
    end

endmodule