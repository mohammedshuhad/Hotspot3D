# FPGA and GPU Optimization for Hotspot 3D kerne
Hotspot 3D is a thermal modeling tool in Rodinia
benchmark suite for heterogeneous computing. It was made faster on FPGA by using memory coalescing, ping pong
buffering, and stencil operation optimization, which improved its performance by 25x compared to CPU. Its performance was improved by 100x on GPU through the use of shared memory and multi-streaming optimizations
