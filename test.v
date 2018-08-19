// Benchmark "test" written by ABC on Sun Aug 19 14:10:32 2018

module test ( 
    CLR, v6, v5, v4, v3, v2, v1, v0, v12, v11, v10, v9, v8, v7,
    finalOutput  );
  input  CLR, v6, v5, v4, v3, v2, v1, v0, v12, v11, v10, v9, v8, v7;
  output finalOutput;
  wire n40, n44, n48, n59, n69, n70, n74, n76_1, n90, n93, n103, n106, n113,
    n127, n134, n160, n161, n199, n213, n227, n331, n384, n419, n556, n557,
    n558, n559, n560, n561, n562, n563, n564, n565, n566, n567, n568, n569,
    n570, n571, n572, n573, n574, n575, n576, n577, n578, n579, n580, n581,
    n582, n583, n584, n585, n586, n587, n588, n589, n590, n591, n592, n593,
    n594, n595, n596, n597, n598, n599, n600, n601, n602, n603, n604, n605,
    n606, n607, n608, n609, n610, n611, n612, n613, n614, n615, n616, n617,
    n618, n71;
  assign n40 = v11 & v10;
  assign n44 = ~v10 & v8;
  assign n48 = ~v12 & ~v9;
  assign n59 = ~v2 & v8;
  assign n69 = ~v12 & v10;
  assign n70 = ~v11 & n69;
  assign n74 = v12 & v10;
  assign n76_1 = v12 & v11;
  assign n90 = ~v12 & v11;
  assign n93 = ~v11 & ~v10;
  assign n103 = ~v11 & v8;
  assign n106 = v9 & ~v8;
  assign n113 = v5 & v4;
  assign n127 = v2 & v10;
  assign n134 = ~v10 & ~v9;
  assign n160 = v11 & v9;
  assign n161 = ~v0 & v11;
  assign n199 = ~v11 & v10;
  assign n213 = v10 & ~v9;
  assign n227 = ~v10 & n76_1;
  assign n331 = ~v11 & v9;
  assign n384 = v12 & v9;
  assign n419 = ~v11 & n48;
  assign n556 = ~v12 & n103;
  assign n557 = ~v6 & ~v7;
  assign n558 = ~v8 & n557;
  assign n559 = v12 & n558;
  assign n560 = ~v12 & v8;
  assign n561 = ~n559 & ~n560;
  assign n562 = ~v3 & ~n561;
  assign n563 = ~n556 & ~n562;
  assign n564 = n134 & ~n563;
  assign n565 = v11 & ~n127;
  assign n566 = v12 & ~n199;
  assign n567 = ~n565 & n566;
  assign n568 = ~v9 & ~n567;
  assign n569 = ~v12 & ~n93;
  assign n570 = ~n568 & ~n569;
  assign n571 = v8 & ~n570;
  assign n572 = n90 & ~n213;
  assign n573 = ~n571 & ~n572;
  assign n574 = v7 & ~n573;
  assign n575 = ~v6 & v9;
  assign n576 = ~v12 & n575;
  assign n577 = n93 & n576;
  assign n578 = n69 & ~n161;
  assign n579 = ~n227 & ~n578;
  assign n580 = ~v9 & ~n579;
  assign n581 = ~n577 & ~n580;
  assign n582 = ~v8 & ~n581;
  assign n583 = v10 & ~v8;
  assign n584 = n48 & n583;
  assign n585 = n44 & n160;
  assign n586 = ~n584 & ~n585;
  assign n587 = ~n113 & ~n586;
  assign n588 = n74 & n331;
  assign n589 = ~v9 & n70;
  assign n590 = v1 & v12;
  assign n591 = ~v9 & ~n590;
  assign n592 = v8 & ~n591;
  assign n593 = v11 & n592;
  assign n594 = ~v11 & n106;
  assign n595 = ~v12 & n594;
  assign n596 = ~n593 & ~n595;
  assign n597 = ~v10 & ~n596;
  assign n598 = ~n589 & ~n597;
  assign n599 = v2 & ~n598;
  assign n600 = n113 & n419;
  assign n601 = n59 & n76_1;
  assign n602 = ~n595 & ~n601;
  assign n603 = ~v1 & ~n602;
  assign n604 = ~n600 & ~n603;
  assign n605 = ~v10 & ~n604;
  assign n606 = v3 & ~v12;
  assign n607 = n93 & n606;
  assign n608 = ~n40 & n384;
  assign n609 = ~n607 & ~n608;
  assign n610 = v8 & ~n609;
  assign n611 = ~n599 & ~n605;
  assign n612 = ~n610 & n611;
  assign n613 = ~n582 & ~n587;
  assign n614 = ~n588 & n613;
  assign n615 = n612 & n614;
  assign n616 = ~v7 & ~n615;
  assign n617 = ~n564 & ~n574;
  assign n618 = ~n616 & n617;
  assign n71 = CLR & ~n618;
  assign finalOutput = n71;
endmodule


