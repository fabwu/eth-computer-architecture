`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 01/24/2021 04:55:17 PM
// Design Name: 
// Module Name: FourBitAdder
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module FourBitAdder(input [3:0] a, input [3:0] b, output [4:0] s);
    wire c[2:0];

    FullAdder fa0 (.a(a[0]), .b(b[0]), .ci(0),  .s(s[0]), .co(c[0]));
    FullAdder fa1 (.a(a[1]), .b(b[1]), .ci(c[0]), .s(s[1]), .co(c[1]));
    FullAdder fa2 (.a(a[2]), .b(b[2]), .ci(c[1]), .s(s[2]), .co(c[2]));
    FullAdder fa3 (.a(a[3]), .b(b[3]), .ci(c[2]), .s(s[3]), .co(s[4]));
endmodule
