`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 01/24/2021 04:49:23 PM
// Design Name: 
// Module Name: FullAdder
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


module FullAdder(input a, input b, input ci, output s, output co);
    assign co = (~ci & a & b) | (ci & ~b & a) | (ci & b & ~a) | (ci & b & a);
    assign s = (~ci & a & ~b) | (~ci & b & ~a) | (ci & ~b & ~a) | (ci & b & a);
endmodule
