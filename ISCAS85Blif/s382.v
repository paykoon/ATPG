// Benchmark "s382.bench" written by ABC on Fri Mar 30 21:44:53 2018

module \s382.bench  ( 
    FM, TEST, CLR, TESTL, FML, OLATCH_Y2L, OLATCHVUC_6, OLATCHVUC_5,
    OLATCH_R1L, OLATCH_G2L, OLATCH_G1L, OLATCH_FEL, C3_Q3, C3_Q2, C3_Q1,
    C3_Q0, UC_16, UC_17, UC_18, UC_19, UC_8, UC_9, UC_10, UC_11,
    GRN1, GRN2, RED1, YLW2, RED2, YLW1, n20, n25, n30, n35, n40, n45, n50,
    n55, n60, n65, n70, n75, n80, n85, n90, n95, n100, n105, n110, n115,
    n120  );
  input  FM, TEST, CLR, TESTL, FML, OLATCH_Y2L, OLATCHVUC_6, OLATCHVUC_5,
    OLATCH_R1L, OLATCH_G2L, OLATCH_G1L, OLATCH_FEL, C3_Q3, C3_Q2, C3_Q1,
    C3_Q0, UC_16, UC_17, UC_18, UC_19, UC_8, UC_9, UC_10, UC_11;
  output GRN1, GRN2, RED1, YLW2, RED2, YLW1, n20, n25, n30, n35, n40, n45,
    n50, n55, n60, n65, n70, n75, n80, n85, n90, n95, n100, n105, n110,
    n115, n120;
  wire n52, n53, n54, n56, n57, n58, n60_1, n61, n62, n64, n65_1, n66, n67,
    n68, n69, n70_1, n71, n72, n73, n75_1, n76, n77, n78, n79, n80_1, n81,
    n82, n83, n85_1, n86, n87, n88, n89, n90_1, n91, n93, n94, n95_1, n96,
    n97, n99, n100_1, n101, n102, n103, n104, n106, n107, n108, n109,
    n110_1, n111, n112, n114, n115_1, n116, n117, n118, n119, n120_1, n121,
    n122, n123, n124, n125, n126, n127, n128, n129, n130, n132, n133, n134,
    n135, n136, n137, n139, n140, n141, n142, n143, n145, n146, n147, n149,
    n150, n151, n152, n153, n155, n156, n157, n158, n159, n160, n162, n163,
    n164, n165, n166, n168, n169, n170, n172, n173, n174, n175, n177, n178,
    n179, n180, n181, n183, n184, n185, n187;
  assign n52 = TEST & TESTL;
  assign n53 = ~TEST & ~TESTL;
  assign n54 = ~n52 & ~n53;
  assign n20 = ~CLR & n54;
  assign n56 = FM & FML;
  assign n57 = ~FM & ~FML;
  assign n58 = ~n56 & ~n57;
  assign n25 = ~CLR & n58;
  assign n60_1 = ~CLR & ~OLATCH_FEL;
  assign n61 = ~C3_Q2 & C3_Q1;
  assign n62 = n60_1 & n61;
  assign n30 = C3_Q0 & n62;
  assign n64 = ~CLR & OLATCH_FEL;
  assign n65_1 = ~C3_Q1 & C3_Q0;
  assign n66 = ~FML & n65_1;
  assign n67 = ~C3_Q3 & C3_Q2;
  assign n68 = n66 & n67;
  assign n69 = n64 & ~n68;
  assign n70_1 = ~CLR & FML;
  assign n71 = C3_Q2 & n70_1;
  assign n72 = ~C3_Q1 & n71;
  assign n73 = ~C3_Q0 & n72;
  assign n60 = n69 | n73;
  assign n75_1 = C3_Q3 & ~C3_Q2;
  assign n76 = ~CLR & n75_1;
  assign n77 = n67 & n70_1;
  assign n78 = ~n76 & ~n77;
  assign n79 = ~C3_Q1 & ~n78;
  assign n80_1 = ~C3_Q0 & n79;
  assign n81 = ~n69 & ~n80_1;
  assign n82 = ~n60 & ~n81;
  assign n83 = ~UC_17 & ~n81;
  assign n35 = ~n82 & ~n83;
  assign n85_1 = ~CLR & C3_Q2;
  assign n86 = ~C3_Q1 & ~C3_Q0;
  assign n87 = ~CLR & C3_Q3;
  assign n88 = n86 & n87;
  assign n89 = ~n85_1 & ~n88;
  assign n90_1 = ~n60 & ~n89;
  assign n91 = ~UC_17 & n60;
  assign n40 = ~n90_1 & ~n91;
  assign n93 = ~C3_Q3 & ~C3_Q2;
  assign n94 = ~OLATCH_FEL & n93;
  assign n95_1 = ~OLATCH_FEL & ~C3_Q2;
  assign n96 = n65_1 & n95_1;
  assign n97 = ~CLR & ~n94;
  assign n45 = n96 | ~n97;
  assign n99 = C3_Q1 & C3_Q0;
  assign n100_1 = ~CLR & n99;
  assign n101 = C3_Q3 & ~C3_Q0;
  assign n102 = ~CLR & n101;
  assign n103 = ~n64 & ~n85_1;
  assign n104 = ~n100_1 & n103;
  assign n50 = ~n102 & n104;
  assign n106 = FML & C3_Q3;
  assign n107 = FML & n86;
  assign n108 = ~n106 & ~n107;
  assign n109 = n85_1 & n108;
  assign n110_1 = ~FML & ~C3_Q3;
  assign n111 = n65_1 & n110_1;
  assign n112 = OLATCH_FEL & ~n111;
  assign n55 = n109 & ~n112;
  assign n114 = ~UC_17 & ~UC_18;
  assign n115_1 = ~UC_19 & n114;
  assign n116 = ~UC_9 & ~UC_10;
  assign n117 = ~UC_11 & n116;
  assign n118 = UC_8 & ~n117;
  assign n119 = ~TESTL & ~n118;
  assign n120_1 = ~n115_1 & ~n119;
  assign n121 = UC_16 & n120_1;
  assign n122 = C3_Q2 & n121;
  assign n123 = C3_Q1 & n122;
  assign n124 = C3_Q0 & n123;
  assign n125 = ~C3_Q3 & ~n124;
  assign n126 = ~C3_Q2 & ~C3_Q1;
  assign n127 = ~C3_Q0 & n126;
  assign n128 = n121 & ~n127;
  assign n129 = C3_Q3 & n128;
  assign n130 = ~CLR & ~n125;
  assign n65 = ~n129 & n130;
  assign n132 = C3_Q1 & n121;
  assign n133 = C3_Q0 & n132;
  assign n134 = ~C3_Q2 & ~n133;
  assign n135 = C3_Q2 & n133;
  assign n136 = ~n134 & ~n135;
  assign n137 = ~CLR & n136;
  assign n70 = ~n129 & n137;
  assign n139 = C3_Q0 & n121;
  assign n140 = ~C3_Q1 & ~n139;
  assign n141 = C3_Q1 & n139;
  assign n142 = ~n140 & ~n141;
  assign n143 = ~CLR & n142;
  assign n75 = ~n129 & n143;
  assign n145 = ~C3_Q0 & ~n121;
  assign n146 = ~n139 & ~n145;
  assign n147 = ~CLR & n146;
  assign n80 = ~n129 & n147;
  assign n149 = UC_17 & ~n119;
  assign n150 = UC_18 & n149;
  assign n151 = UC_19 & n150;
  assign n152 = ~UC_16 & ~n151;
  assign n153 = ~CLR & ~n152;
  assign n85 = ~n121 & n153;
  assign n155 = UC_18 & ~n119;
  assign n156 = UC_19 & n155;
  assign n157 = ~UC_17 & ~n156;
  assign n158 = UC_17 & n156;
  assign n159 = ~n157 & ~n158;
  assign n160 = ~CLR & n159;
  assign n90 = ~n121 & n160;
  assign n162 = UC_19 & ~n119;
  assign n163 = ~UC_18 & ~n162;
  assign n164 = UC_18 & n162;
  assign n165 = ~n163 & ~n164;
  assign n166 = ~CLR & n165;
  assign n95 = ~n121 & n166;
  assign n168 = ~UC_19 & n119;
  assign n169 = ~n162 & ~n168;
  assign n170 = ~CLR & n169;
  assign n100 = ~n121 & n170;
  assign n172 = UC_9 & UC_10;
  assign n173 = UC_11 & n172;
  assign n174 = ~UC_8 & ~n173;
  assign n175 = ~CLR & ~n174;
  assign n105 = ~n118 & n175;
  assign n177 = UC_10 & UC_11;
  assign n178 = ~UC_9 & ~n177;
  assign n179 = UC_9 & n177;
  assign n180 = ~n178 & ~n179;
  assign n181 = ~CLR & n180;
  assign n110 = ~n118 & n181;
  assign n183 = ~UC_10 & ~UC_11;
  assign n184 = ~n177 & ~n183;
  assign n185 = ~CLR & n184;
  assign n115 = ~n118 & n185;
  assign n187 = ~CLR & ~UC_11;
  assign n120 = ~n118 & n187;
  assign RED2 = ~OLATCHVUC_5;
  assign YLW1 = ~OLATCHVUC_6;
  assign GRN1 = OLATCH_G1L;
  assign GRN2 = OLATCH_G2L;
  assign RED1 = OLATCH_R1L;
  assign YLW2 = OLATCH_Y2L;
endmodule


