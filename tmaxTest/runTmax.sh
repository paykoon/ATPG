#!/bin/sh

rm ./dataFile/tmaxPattern

tmax -shell << EOF
read_netlist lib.v
read_netlist ./dataFile/test.v
run_build_model
run_drc
set_faults -model stuck
# add_faults -all
read_faults ./dataFile/test.faults
set_atpg -coverage 100 -abort_limit 100 -merge high
run_atpg -auto_compression
report_patterns -all >> ./dataFile/tmaxPattern
report_faults -all >> ./dataFile/tmaxResults
exit
<< EOF

rm tmaxPattern

