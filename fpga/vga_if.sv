/*

"data_in" connected to "data_out" from spi_controller

X, Y coordinates (800 x 525) require 10 bits each to represent.
Since SPI master can only send one byte (eight bits) per transaction,
two bytes are sent. The second sent byte contains the two MSB bits.


data_in[0]: lower eight bits of ball_x
data_in[1]: first two bits (0, 1) are MSB of ball_x

data_in[2]: lower eight bits of ball_y
data_in[3]: first two bits (0, 1) are MSB of ball_y

data_in[4]: lower eight bits of paddle_l_y
data_in[5]: first two bits (0, 1) are MSB of paddle_l_y

data_in[6]: lower eight bits of paddle_r_y
data_in[7]: first two bits (0, 1) are MSB of paddle_r_y

data_in[8]: score_l

data_in[9]: score_r

*/

`timescale 1ns/1ps

module vga_if (
    input logic sys_clk_i,
    input logic data_ready_i,

    input logic [7:0] data_in [0:9],

    output logic [9:0] ball_x,
    output logic [9:0] ball_y,
    output logic [9:0] paddle_l_y,
    output logic [9:0] paddle_r_y,
    output logic [7:0] score_l,
    output logic [7:0] score_r
    
    );

    always_ff @(posedge sys_clk_i) begin
        if (data_ready_i) begin
            ball_x <= {data_in[1][1:0], data_in[0]};
            ball_y <= {data_in[3][1:0], data_in[2]};
            paddle_l_y <= {data_in[5][1:0], data_in[4]};
            paddle_r_y <= {data_in[7][1:0], data_in[6]};
            score_l <= data_in[8];
            score_r <= data_in[9];
        end
    end

endmodule
