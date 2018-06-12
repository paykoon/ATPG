// Benchmark "aig/s444" written by ABC on Wed Mar 28 15:36:49 2018

module \aig/s444  ( 
    G0, G1, G2, G11, G12, G13, G14, G15, G16, G17, G18, G19, G20, G21, G22,
    G23, G24, G25, G26, G27, G28, G29, G30, G31,
    G118, G167, G107, G119, G168, G108, n20, n25, n30, n35, n40, n45, n50,
    n55, n60, n65, n70, n75, n80, n85, n90, n95, n100, n105, n110, n115,
    n120  );
  input  G0, G1, G2, G11, G12, G13, G14, G15, G16, G17, G18, G19, G20,
    G21, G22, G23, G24, G25, G26, G27, G28, G29, G30, G31;
  output G118, G167, G107, G119, G168, G108, n20, n25, n30, n35, n40, n45,
    n50, n55, n60, n65, n70, n75, n80, n85, n90, n95, n100, n105, n110,
    n115, n120;
  wire n52, n53, n54, n55_1, n57, n58, n59, n60_1, n61, n63, n64, n65_1, n66,
    n67, n69, n70_1, n71, n72, n73, n75_1, n76, n77, n78, n79, n80_1, n81,
    n82, n83, n84, n86, n87, n88, n89, n90_1, n92, n93, n94, n95_1, n96,
    n97, n98, n100_1, n101, n102, n103, n104, n105_1, n106, n108, n109,
    n110_1, n111, n112, n113, n114, n115_1, n116, n118, n119, n120_1, n121,
    n122, n124, n125, n126, n127, n128, n129, n130, n132, n133, n134, n135,
    n136, n137, n138, n140, n141, n142, n144, n145, n146, n147, n148, n149,
    n150, n151, n152, n153, n155, n156, n157, n158, n159, n160, n161, n162,
    n163, n165, n166, n167, n168, n169, n170, n171, n173, n174, n175, n176,
    n177, n178, n180, n181, n182, n183, n184, n185, n186, n188, n189, n190,
    n191, n192, n193, n194, n195, n196, n197, n199, n200, n201, n203, n204,
    n205;
  assign n52 = ~G11 & ~G12;
  assign n53 = ~G13 & n52;
  assign n54 = G14 & ~n53;
  assign n55_1 = ~G11 & ~n54;
  assign n20 = ~G0 & n55_1;
  assign n57 = G11 & G12;
  assign n58 = G12 & ~n57;
  assign n59 = G11 & ~n57;
  assign n60_1 = ~n58 & ~n59;
  assign n61 = ~n54 & ~n60_1;
  assign n25 = ~G0 & n61;
  assign n63 = G13 & n57;
  assign n64 = G13 & ~n63;
  assign n65_1 = n57 & ~n63;
  assign n66 = ~n64 & ~n65_1;
  assign n67 = ~n54 & ~n66;
  assign n30 = ~G0 & n67;
  assign n69 = G14 & n63;
  assign n70_1 = G14 & ~n69;
  assign n71 = n63 & ~n69;
  assign n72 = ~n70_1 & ~n71;
  assign n73 = ~n54 & ~n72;
  assign n35 = ~G0 & n73;
  assign n75_1 = ~G31 & ~n54;
  assign n76 = ~G15 & ~G16;
  assign n77 = ~G17 & n76;
  assign n78 = G18 & ~n75_1;
  assign n79 = ~n77 & n78;
  assign n80_1 = G15 & ~n75_1;
  assign n81 = G15 & ~n80_1;
  assign n82 = ~n75_1 & ~n80_1;
  assign n83 = ~n81 & ~n82;
  assign n84 = ~n79 & ~n83;
  assign n40 = ~G0 & n84;
  assign n86 = G16 & n80_1;
  assign n87 = G16 & ~n86;
  assign n88 = n80_1 & ~n86;
  assign n89 = ~n87 & ~n88;
  assign n90_1 = ~n79 & ~n89;
  assign n45 = ~G0 & n90_1;
  assign n92 = G15 & G16;
  assign n93 = ~n75_1 & n92;
  assign n94 = G17 & n93;
  assign n95_1 = G17 & ~n94;
  assign n96 = n93 & ~n94;
  assign n97 = ~n95_1 & ~n96;
  assign n98 = ~n79 & ~n97;
  assign n50 = ~G0 & n98;
  assign n100_1 = G17 & n92;
  assign n101 = ~n75_1 & n100_1;
  assign n102 = G18 & n101;
  assign n103 = G18 & ~n102;
  assign n104 = n101 & ~n102;
  assign n105_1 = ~n103 & ~n104;
  assign n106 = ~n79 & ~n105_1;
  assign n55 = ~G0 & n106;
  assign n108 = ~G19 & ~G20;
  assign n109 = ~G21 & n108;
  assign n110_1 = G22 & n79;
  assign n111 = ~n109 & n110_1;
  assign n112 = G19 & n79;
  assign n113 = G19 & ~n112;
  assign n114 = n79 & ~n112;
  assign n115_1 = ~n113 & ~n114;
  assign n116 = ~n111 & ~n115_1;
  assign n60 = ~G0 & n116;
  assign n118 = G20 & n112;
  assign n119 = G20 & ~n118;
  assign n120_1 = n112 & ~n118;
  assign n121 = ~n119 & ~n120_1;
  assign n122 = ~n111 & ~n121;
  assign n65 = ~G0 & n122;
  assign n124 = G19 & G20;
  assign n125 = n79 & n124;
  assign n126 = G21 & n125;
  assign n127 = G21 & ~n126;
  assign n128 = n125 & ~n126;
  assign n129 = ~n127 & ~n128;
  assign n130 = ~n111 & ~n129;
  assign n70 = ~G0 & n130;
  assign n132 = G21 & n124;
  assign n133 = n79 & n132;
  assign n134 = G22 & n133;
  assign n135 = G22 & ~n134;
  assign n136 = n133 & ~n134;
  assign n137 = ~n135 & ~n136;
  assign n138 = ~n111 & ~n137;
  assign n75 = ~G0 & n138;
  assign n140 = ~G2 & ~G23;
  assign n141 = G2 & G23;
  assign n142 = ~n140 & ~n141;
  assign n80 = ~G0 & n142;
  assign n144 = ~G20 & G21;
  assign n145 = G23 & n144;
  assign n146 = ~G0 & n145;
  assign n147 = ~G19 & n146;
  assign n148 = G19 & ~G20;
  assign n149 = G21 & ~G22;
  assign n150 = ~G23 & n149;
  assign n151 = n148 & n150;
  assign n152 = ~G0 & G24;
  assign n153 = ~n151 & n152;
  assign n85 = n147 | n153;
  assign n155 = ~G22 & n148;
  assign n156 = ~G23 & n155;
  assign n157 = G24 & ~n156;
  assign n158 = ~G20 & G23;
  assign n159 = ~G19 & n158;
  assign n160 = G22 & G23;
  assign n161 = ~n159 & ~n160;
  assign n162 = G21 & n161;
  assign n163 = ~G0 & n162;
  assign n90 = ~n157 & n163;
  assign n165 = ~G0 & G22;
  assign n166 = ~G19 & n165;
  assign n167 = ~G0 & G20;
  assign n168 = G19 & n167;
  assign n169 = ~G0 & G21;
  assign n170 = ~n166 & ~n168;
  assign n171 = ~n169 & n170;
  assign n95 = ~n152 & n171;
  assign n173 = ~G21 & ~G24;
  assign n174 = ~G20 & n173;
  assign n175 = G19 & n174;
  assign n176 = ~G22 & ~G24;
  assign n177 = ~G21 & n176;
  assign n178 = ~n175 & ~n177;
  assign n100 = G0 | ~n178;
  assign n180 = ~G20 & n165;
  assign n181 = ~G19 & n180;
  assign n182 = ~n152 & ~n181;
  assign n183 = ~n169 & n182;
  assign n184 = ~G17 & ~n183;
  assign n185 = n85 & n184;
  assign n186 = ~n85 & ~n183;
  assign n105 = ~n185 & ~n186;
  assign n188 = ~G0 & G23;
  assign n189 = ~G22 & n188;
  assign n190 = G21 & n189;
  assign n191 = ~G21 & n165;
  assign n192 = ~n190 & ~n191;
  assign n193 = n108 & ~n192;
  assign n194 = ~n153 & ~n193;
  assign n195 = ~G17 & ~n194;
  assign n196 = n85 & n195;
  assign n197 = ~n85 & ~n194;
  assign n110 = ~n196 & ~n197;
  assign n199 = G20 & ~G21;
  assign n200 = ~G24 & n199;
  assign n201 = ~G0 & n200;
  assign n115 = G19 & n201;
  assign n203 = ~G1 & ~G31;
  assign n204 = G1 & G31;
  assign n205 = ~n203 & ~n204;
  assign n120 = ~G0 & n205;
  assign G167 = ~G29;
  assign G119 = ~G28;
  assign G118 = G27;
  assign G107 = G25;
  assign G168 = G30;
  assign G108 = G26;
endmodule


