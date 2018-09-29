module test(G0, G1, G2, G3, G5, G6, G7, G17, n12, n17, n22);




input G0, G1, G2, G3, G5, G6, G7;




output G17, n12, n17, n22; 




wire n12_1, n13, n14, n15, n16, n17, n12, n22, G17;




N3 g7(n12_1, G0, G6);
N7 g8(n13, G3, n12_1);
N7 g9(n14, G1, G7);
N7 g10(n15, n12_1, n14);
N7 g11(n16, n13, n15);
N3 g12(n17, G5, n16);
N2 g13(n12, G0, n17);
N7 g14(n22, G2, n14);
N10 g15(G17, n17);





endmodule
