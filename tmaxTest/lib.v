module N1(x,y,z);
   input y,z;
   output x;
   assign x = (y&z);
endmodule

module N2(x,y,z);
   input y,z;
   output x;
   assign x = (y&~z);
endmodule

module N3(x,y,z);
   input y,z;
   output x;
   assign x = (~y&z);
endmodule


module N4(x,y,z);
   input y,z;
   output x;
   assign x = ~(y&z);
endmodule

module N5(x,y,z);
   input y,z;
   output x;
   assign x = ~(y&~z);
endmodule

module N6(x,y,z);
   input y,z;
   output x;
   assign x = ~(~y&z);
endmodule


module N7(x,y,z);
   input y,z;
   output x;
   assign x = (~y&~z);
endmodule


module N8(x,y,z);
   input y,z;
   output x;
   assign x = ~(~y&~z);
endmodule

module N9(x,y);
   input y;
   output x;
   assign x = y;
endmodule

module N10(x,y);
   input y;
   output x;
   assign x = ~y;
endmodule

module N11(x);
   output x;
   assign x = 1;
endmodule

module N12(x);
   output x;
   assign x = 0;
endmodule
