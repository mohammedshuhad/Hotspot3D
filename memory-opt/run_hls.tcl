#===================================
# run_hls.tcl for 3D
#===================================
# open the HLS project 3D.prj
open_project 3D.prj -reset
# set the top-level function of the design to be 3D
set_top workload
# add design files
add_files 3D.cpp
# add the testbench files
add_files -tb "3D_test.cpp power_512x8 temp_512x8 debug_power_10x4 debug_temp_10x4"

# open HLS solution solution1
open_solution "solution1"
# set target FPGA device: Alveo U50 in this example
set_part {xcu50-fsvh2104-2-e}
# target clock period is 5 ns, i.e., 200MHz
create_clock -period 3.33

# do a c simulation
csim_design
# synthesize the design
csynth_design
# do a co-simulation
#cosim_design
# close project and quit
close_project
# exit Vivado HLS
quit
