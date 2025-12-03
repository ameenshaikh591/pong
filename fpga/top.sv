`timescale 1ns / 1ps

module top(

    input logic reset_i,
    
    input logic sys_clk_i,
    
    input logic sclk_i,
    input logic mosi_i,
    input logic chip_enable_i,
    
    output logic hsync_o,
    output logic vsync_o,
    output logic [3:0] r,
    output logic [3:0] g,
    output logic [3:0] b
    
);
    
    logic [7:0] spi_data [0:9];
    logic data_ready;
    
    spi_controller spi(
        .sys_clk_i(sys_clk_i),
        .sclk_i(sclk_i),
        .mosi_i(mosi_i),
        .chip_enable_i(chip_enable_i),
        .data_o(spi_data),
        .data_ready_o(data_ready)
    );
    
    logic [9:0] ball_x;
    logic [9:0] ball_y;
    logic [9:0] paddle_l_y;
    logic [9:0] paddle_r_y;
    logic [7:0] score_l;
    logic [7:0] score_r;
    
    
    vga_if spi_vga_if(
        .sys_clk_i(sys_clk_i),
        .data_ready_i(data_ready),
        .data_in(spi_data),
        .ball_x(ball_x),
        .ball_y(ball_y),
        .paddle_l_y(paddle_l_y),
        .paddle_r_y(paddle_r_y),
        .score_l(score_l),
        .score_r(score_r)
    );
    
    logic [9:0] x;
    logic [9:0] y;
    logic hsync;
    logic vsync;
    logic video_on;
    
    vga_timing sync(
        .sys_clk_i(sys_clk_i),
        .reset_i(reset_i),

        .x(x),
        .y(y),
        .hsync(hsync),
        .vsync(vsync),
        .video_on(video_on)
    );
    
    vga_renderer renderer(
        .x(x),
        .y(y),
        .video_on(video_on),
    
        .ball_x(ball_x),
        .ball_y(ball_y),
        .paddle_l_y(paddle_l_y),
        .paddle_r_y(paddle_r_y),
        .score_l(score_l),
        .score_r(score_r),
    
        .r(r),
        .g(g),
        .b(b)
    );
    
    always_comb begin
        hsync_o = hsync;
        vsync_o = vsync;
    end
    
    
    
endmodule
