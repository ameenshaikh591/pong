`timescale 1ns/1ps

/*

Pin mappings:
A14 --> SCLK --> sclk_i
B15 --> MOSI --> mosi_i
B16 --> CS --> chip_enable_i (active high)

*/

module spi_controller (
    input logic sys_clk_i,
    input logic sclk_i,
    input logic mosi_i,
    input logic chip_enable_i,
    output logic [7:0] data_o [0:9],
    output logic data_ready_o
);

    localparam BYTES_PER_TRANSACTION = 10;

    logic [1:0] ce_sync, sclk_sync; // 2-FF synchronizers to handle metastability
    logic ce_sync_d, sclk_sync_d; // For edge detection
    logic ce_rise, sclk_rise, valid_sample;

    logic [7:0] data;
    logic [2:0] bit_counter;
    logic [3:0] byte_counter;

    always_ff @(posedge sys_clk_i) begin
        ce_sync <= {ce_sync[0], chip_enable_i};
        ce_sync_d <= ce_sync[1];

        sclk_sync <= {sclk_sync[0], sclk_i};
        sclk_sync_d <= sclk_sync[1];
    end

    always_comb begin
        ce_rise = ce_sync[1] & ~ce_sync_d;
        sclk_rise = sclk_sync[1] & ~sclk_sync_d;
        valid_sample = sclk_rise & ce_sync[1]; 
    end

    always_ff @(posedge sys_clk_i) begin

        if (ce_rise) begin
            bit_counter <= 3'd0;
            byte_counter <= 3'd0;
            data <= 8'd0;
            data_ready_o <= 1'b0;

            for (int i = 0; i < BYTES_PER_TRANSACTION; i++) begin
                data_o[i] <= 8'd0;
            end
        end

        if (valid_sample && byte_counter < BYTES_PER_TRANSACTION) begin

            data <= {data[6:0], mosi_i};

            if (bit_counter == 3'd7) begin
                data_o[byte_counter] <= {data[6:0], mosi_i};

                bit_counter  <= 3'd0;
                byte_counter <= byte_counter + 1;

                if (byte_counter == BYTES_PER_TRANSACTION - 1) begin
                    data_ready_o <= 1'b1;
                end

            end else begin
                bit_counter <= bit_counter + 1;
            end
        end
    end

endmodule
