#!/bin/sh

tmax -shell << EOF
read_netlist ./lib.v
read_netlist ./dataFile/test.v
run_build_model
run_drc
set_faults -model stuck
read_faults ./dataFile/test.faults
run_atpg
report_faults -class UR >> ./dataFile/faults.report

