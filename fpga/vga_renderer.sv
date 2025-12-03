`timescale 1ns/1ps

/*

*/

module vga_renderer (
    input  logic [9:0] x,
    input  logic [9:0] y,
    input  logic       video_on,

    input  logic [9:0] ball_x,
    input  logic [9:0] ball_y,
    input  logic [9:0] paddle_l_y,
    input  logic [9:0] paddle_r_y,
    input  logic [7:0] score_l,
    input  logic [7:0] score_r,

    output logic [3:0] r,
    output logic [3:0] g,
    output logic [3:0] b
);

    localparam X_MAX = 639;

    localparam BALL_WIDTH  = 10;
    localparam BALL_LENGTH = 10;
    localparam FULL_RED    = 4'hF;

    localparam PADDLE_WIDTH  = 10;
    localparam PADDLE_LENGTH = 60;

    // Score box parameters
    localparam SCORE_BOX_WIDTH  = 25;
    localparam SCORE_BOX_LENGTH = 50;
    localparam SCORE_BOX_L_X    = 310;
    localparam SCORE_BOX_R_X    = 345;
    localparam SCORE_BOX_Y      = 10;

    // Segment thickness (in pixels)
    localparam SEG_THICK = 4;

    // 7-seg pattern: {a,b,c,d,e,f,g}
    function automatic logic [6:0] seg7 (input logic [3:0] val);
        case (val)
            4'd0: seg7 = 7'b1111110;
            4'd1: seg7 = 7'b0110000;
            4'd2: seg7 = 7'b1101101;
            4'd3: seg7 = 7'b1111001;
            4'd4: seg7 = 7'b0110011;
            4'd5: seg7 = 7'b1011011;
            4'd6: seg7 = 7'b1011111;
            4'd7: seg7 = 7'b1110000;
            4'd8: seg7 = 7'b1111111;
            4'd9: seg7 = 7'b1111011;
            default: seg7 = 7'b0000000;
        endcase
    endfunction

    // Horizontal segment hit test
    function automatic logic hit_hseg (
        input int seg_x,
        input int seg_y,
        input int width,
        input int thick,
        input logic [9:0] px,
        input logic [9:0] py
    );
        hit_hseg = (px >= seg_x) && (px < seg_x + width) &&
                   (py >= seg_y) && (py < seg_y + thick);
    endfunction

    // Vertical segment hit test
    function automatic logic hit_vseg (
        input int seg_x,
        input int seg_y,
        input int thick,
        input int length,
        input logic [9:0] px,
        input logic [9:0] py
    );
        hit_vseg = (px >= seg_x) && (px < seg_x + thick) &&
                   (py >= seg_y) && (py < seg_y + length);
    endfunction

    logic show_score_l;
    logic show_score_r;
    logic show_scores;       // NEW: combined flag

    logic [3:0] digit_l;
    logic [3:0] digit_r;
    logic [6:0] seg_l;
    logic [6:0] seg_r;

    // Left box origin
    int lx;
    int ly;

    // Right box origin
    int rx;
    int ry;

    always_comb begin
        // Default black
        r = 4'd0;
        g = 4'd0;
        b = 4'd0;

        if (video_on) begin

            // Red Ball
            if (x >= ball_x && x < ball_x + BALL_WIDTH) begin
                if (y >= ball_y && y < ball_y + BALL_LENGTH) begin
                    r = FULL_RED;
                end
            end

            // Red Left Paddle
            if (y >= paddle_l_y && y < paddle_l_y + PADDLE_LENGTH) begin
                if (x >= 0 && x < PADDLE_WIDTH) begin
                    r = FULL_RED;
                end
            end

            // Red Right Paddle
            if (y >= paddle_r_y && y < paddle_r_y + PADDLE_LENGTH) begin
                if (x > X_MAX - PADDLE_WIDTH && x <= X_MAX) begin
                    r = FULL_RED;
                end
            end

            // -------------------------------------------------
            // SCORE OVERLAY (7-segment style), on top of ball/paddles
            // MSB of score_l / score_r is "display" flag
            // lower 4 bits are the digit (0-9)
            // Both scores are displayed if EITHER MSB is set
            // -------------------------------------------------

            show_score_l = score_l[7];
            show_score_r = score_r[7];
            show_scores  = show_score_l | show_score_r;

            digit_l = score_l[3:0];
            digit_r = score_r[3:0];

            seg_l = seg7(digit_l);
            seg_r = seg7(digit_r);

            lx = SCORE_BOX_L_X;
            ly = SCORE_BOX_Y;

            rx = SCORE_BOX_R_X;
            ry = SCORE_BOX_Y;

            // -------------------------------
            // DRAW LEFT SCORE (if either MSB is set)
            // Digit is fully contained within the box:
            // x in [lx, lx + SCORE_BOX_WIDTH)
            // y in [ly, ly + SCORE_BOX_LENGTH)
            // -------------------------------
            if (show_scores) begin
                // a (top horizontal)
                if (seg_l[6] &&
                    hit_hseg(lx + 4,
                             ly + 2,
                             SCORE_BOX_WIDTH - 8,
                             SEG_THICK,
                             x, y)) begin
                    r = FULL_RED;
                end

                // b (upper right vertical)
                if (seg_l[5] &&
                    hit_vseg(lx + SCORE_BOX_WIDTH - SEG_THICK - 2,
                             ly + 5,
                             SEG_THICK,
                             (SCORE_BOX_LENGTH / 2) - 6,
                             x, y)) begin
                    r = FULL_RED;
                end

                // c (lower right vertical)
                if (seg_l[4] &&
                    hit_vseg(lx + SCORE_BOX_WIDTH - SEG_THICK - 2,
                             ly + (SCORE_BOX_LENGTH / 2) + 1,
                             SEG_THICK,
                             (SCORE_BOX_LENGTH / 2) - 6,
                             x, y)) begin
                    r = FULL_RED;
                end

                // d (bottom horizontal)
                if (seg_l[3] &&
                    hit_hseg(lx + 4,
                             ly + SCORE_BOX_LENGTH - SEG_THICK - 2,
                             SCORE_BOX_WIDTH - 8,
                             SEG_THICK,
                             x, y)) begin
                    r = FULL_RED;
                end

                // e (lower left vertical)
                if (seg_l[2] &&
                    hit_vseg(lx + 2,
                             ly + (SCORE_BOX_LENGTH / 2) + 1,
                             SEG_THICK,
                             (SCORE_BOX_LENGTH / 2) - 6,
                             x, y)) begin
                    r = FULL_RED;
                end

                // f (upper left vertical)
                if (seg_l[1] &&
                    hit_vseg(lx + 2,
                             ly + 5,
                             SEG_THICK,
                             (SCORE_BOX_LENGTH / 2) - 6,
                             x, y)) begin
                    r = FULL_RED;
                end

                // g (middle horizontal)
                if (seg_l[0] &&
                    hit_hseg(lx + 4,
                             ly + (SCORE_BOX_LENGTH / 2) - 2,
                             SCORE_BOX_WIDTH - 8,
                             SEG_THICK,
                             x, y)) begin
                    r = FULL_RED;
                end
            end

            // -------------------------------
            // DRAW RIGHT SCORE (if either MSB is set)
            // Also fully contained in its box
            // -------------------------------
            if (show_scores) begin
                // a (top horizontal)
                if (seg_r[6] &&
                    hit_hseg(rx + 4,
                             ry + 2,
                             SCORE_BOX_WIDTH - 8,
                             SEG_THICK,
                             x, y)) begin
                    r = FULL_RED;
                end

                // b (upper right vertical)
                if (seg_r[5] &&
                    hit_vseg(rx + SCORE_BOX_WIDTH - SEG_THICK - 2,
                             ry + 5,
                             SEG_THICK,
                             (SCORE_BOX_LENGTH / 2) - 6,
                             x, y)) begin
                    r = FULL_RED;
                end

                // c (lower right vertical)
                if (seg_r[4] &&
                    hit_vseg(rx + SCORE_BOX_WIDTH - SEG_THICK - 2,
                             ry + (SCORE_BOX_LENGTH / 2) + 1,
                             SEG_THICK,
                             (SCORE_BOX_LENGTH / 2) - 6,
                             x, y)) begin
                    r = FULL_RED;
                end

                // d (bottom horizontal)
                if (seg_r[3] &&
                    hit_hseg(rx + 4,
                             ry + SCORE_BOX_LENGTH - SEG_THICK - 2,
                             SCORE_BOX_WIDTH - 8,
                             SEG_THICK,
                             x, y)) begin
                    r = FULL_RED;
                end

                // e (lower left vertical)
                if (seg_r[2] &&
                    hit_vseg(rx + 2,
                             ry + (SCORE_BOX_LENGTH / 2) + 1,
                             SEG_THICK,
                             (SCORE_BOX_LENGTH / 2) - 6,
                             x, y)) begin
                    r = FULL_RED;
                end

                // f (upper left vertical)
                if (seg_r[1] &&
                    hit_vseg(rx + 2,
                             ry + 5,
                             SEG_THICK,
                             (SCORE_BOX_LENGTH / 2) - 6,
                             x, y)) begin
                    r = FULL_RED;
                end

                // g (middle horizontal)
                if (seg_r[0] &&
                    hit_hseg(rx + 4,
                             ry + (SCORE_BOX_LENGTH / 2) - 2,
                             SCORE_BOX_WIDTH - 8,
                             SEG_THICK,
                             x, y)) begin
                    r = FULL_RED;
                end
            end

        end // if (video_on)

    end

endmodule

