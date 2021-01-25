`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 01/24/2021 05:26:24 PM
// Design Name: 
// Module Name: testbench
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


module testbench();
    reg [3:0] a, b;
    wire [4:0] out;
 
  FourBitAdder DUT (
    .a(a),
    .b(b),
    .s(out)
  );
 
  initial begin
    a = 4'd2;
    b = 4'd2;
    #20
    $finish;
  end
endmodule
