#!/bin/sh

rm ./dataFile/tmaxPattern

tmax -shell << EOF
read_netlist ./dataFile/test.v
read_netlist lib.v
run_build_model
run_drc
set_faults -model stuck
add_faults -all
# set_atpg -abort_limit 100 -merge high
# run_atpg -auto_compression
set_atpg -coverage 100 -abort_limit 100 -merge high
run_atpg -auto_compression
report_patterns -all >> ./dataFile/tmaxPattern
exit
<< EOF

rm tmaxPattern

