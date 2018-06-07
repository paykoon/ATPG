// Benchmark "test" written by ABC on Thu May 17 16:26:49 2018

module test ( 
    G0, G11, G12, G13, G14,
    finalOutput  );
  input  G0, G11, G12, G13, G14;
  output finalOutput;
  wire n52, n53, n54, n57, n63, n69, n70_1, n71, n72, n73, n35;
  assign n52 = ~G11 & ~G12;
  assign n53 = ~G13 & n52;
  assign n54 = G14 & ~n53;
  assign n57 = G11 & G12;
  assign n63 = G13 & n57;
  assign n69 = G14 & n63;
  assign n70_1 = G14 & ~n69;
  assign n71 = n63 & ~n69;
  assign n72 = ~n70_1 & ~n71;
  assign n73 = ~n54 & ~n72;
  assign n35 = ~G0 & n73;
  assign finalOutput = n35;
endmodule


