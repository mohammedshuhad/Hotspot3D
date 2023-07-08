#ifndef THREED_H
#define THREED_H
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define STR_SIZE (256)
#define MAX_PD	(3.0e6)
/* required precision in degrees  */
#define PRECISION 0.001
#define SPEC_HEAT_SI 1.75e6
#define K_SI 100
/* capacitance fitting factor */
#define FACTOR_CHIP 0.5
#define OPEN
/* chip parameters  */
#define T_CHIP 0.0005
#define CHIP_HEIGHT 0.016
#define CHIP_WIDTH 0.016
#define AMB_TEMP 80.0
#define ITERATIONS 100
#define GRID_ROWS 512
#define GRID_COLS 512
#define GRID_LAYERS 8
#define TILE_LAYERS 1
#define TOP 0
#define BOTTOM (GRID_LAYERS - TILE_LAYERS)
#define TYPE float
#define  dx  CHIP_HEIGHT/GRID_ROWS
#define  dy  CHIP_HEIGHT/GRID_ROWS
#define  dz  T_CHIP/GRID_LAYERS
#define  max_slope ( MAX_PD / (FACTOR_CHIP * T_CHIP * SPEC_HEAT_SI))
#define  dt ( PRECISION / max_slope)
#define  Cap  FACTOR_CHIP * SPEC_HEAT_SI * T_CHIP * dx * dy
#define  Rx  dy / (2.0 * K_SI * T_CHIP * dx)
#define  Ry  dx / (2.0 * K_SI * T_CHIP * dy)
#define  Rz  dz / (K_SI * dx * dy)
#define stepDivCap  (dt / Cap)
#define ce (stepDivCap / Rx)
#define cw (stepDivCap / Rx)
#define cn (stepDivCap / Ry)
#define cs (stepDivCap / Ry)
#define ct (stepDivCap / Rz)
#define cb (stepDivCap / Rz)
#define cc  (1.0 - (2.0 * ce + 2.0 * cn + 3.0 * ct))
#define TILE_FACTOR 8
#define size GRID_ROWS * GRID_COLS * GRID_LAYERS

extern "C" void workload(float tempIn[GRID_ROWS * GRID_COLS* GRID_LAYERS], 
              float powerIn[GRID_ROWS * GRID_COLS* GRID_LAYERS],
              float tempOut[GRID_ROWS * GRID_COLS * GRID_LAYERS]);


#endif