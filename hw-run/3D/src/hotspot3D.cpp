#include"hotspot3D.h"

void compute(float pIn[TILE_LAYERS * GRID_ROWS * GRID_COLS], float tIn[(TILE_LAYERS + 2) * (GRID_ROWS)*GRID_COLS], float tOut[TILE_LAYERS * GRID_ROWS * GRID_COLS], int l)
{
    int  w, e, n, s, b, t;
    int z,r,c;

    Z_LOOP:for (  z = 0 ;  z < TILE_LAYERS ; z++)
        R_LOOP:for ( r = 0; r < GRID_ROWS; r++)
            C_LOOP:for ( c = 0; c < GRID_COLS; c++) {
                
                float center = tIn[(z+1) * GRID_ROWS * GRID_COLS  + r * GRID_COLS + c];

                float bottom = ((l == TOP) && z == 0) ? center : tIn[z * GRID_ROWS * GRID_COLS  + r * GRID_COLS + c];
                float top = ((l == BOTTOM) && z == TILE_LAYERS-1) ? center : tIn[(z+2) * GRID_ROWS * GRID_COLS  + r * GRID_COLS + c];

                float west = ((c ==0 ) ? center : tIn[(z+1) * GRID_ROWS * GRID_COLS  + r * GRID_COLS + c-1] );
                float east = ((c ==GRID_COLS-1 ) ? center : tIn[(z+1) * GRID_ROWS * GRID_COLS  + r * GRID_COLS + c+1] );

                float north = ((r ==0 ) ? center : tIn[(z+1) * GRID_ROWS * GRID_COLS  + (r-1) * GRID_COLS + c] );
                float south = ((r ==GRID_ROWS-1 ) ? center : tIn[(z+1) * GRID_ROWS * GRID_COLS  + (r+1) * GRID_COLS + c] );

                tOut[z* GRID_ROWS * GRID_COLS  + r * GRID_COLS + c] = center * cc + north * cn + south * cs + east * ce + west * cw + top * ct + bottom * cb + (dt / Cap) * pIn[z* GRID_ROWS * GRID_COLS  + r * GRID_COLS + c] + ct * AMB_TEMP;

        }
}
void load(float *temp_inner,float *power_inner, float *tempIn, float *powerIn, int l)
{
  memcpy(temp_inner, tempIn + ((l-1) * GRID_ROWS * GRID_COLS), sizeof(float) * (TILE_LAYERS+2) * (GRID_ROWS)*GRID_COLS);
  memcpy(power_inner, powerIn + (l * GRID_ROWS * GRID_COLS), sizeof(float) * TILE_LAYERS * GRID_ROWS * GRID_COLS);
}

void store(float *tempOut, float *result_inner, int l)
{
  memcpy(tempOut + l * GRID_ROWS * GRID_COLS, result_inner, sizeof(float) * TILE_LAYERS * GRID_ROWS * GRID_COLS);
}

extern "C" {
    void hotspot3D(float powerIn[GRID_ROWS * GRID_COLS * GRID_LAYERS],
              float tempIn[GRID_ROWS * GRID_COLS * GRID_LAYERS],
              float tempOut[GRID_ROWS * GRID_COLS * GRID_LAYERS]
              )
{

#pragma HLS INTERFACE m_axi port = tempIn offset = slave bundle = gmem
#pragma HLS INTERFACE m_axi port = powerIn offset = slave bundle = gmem
#pragma HLS INTERFACE m_axi port = tempOut offset = slave bundle = gmem

#pragma HLS INTERFACE s_axilite port = tempIn bundle = control
#pragma HLS INTERFACE s_axilite port = powerIn bundle = control
#pragma HLS INTERFACE s_axilite port = tempOut bundle = control

#pragma HLS INTERFACE s_axilite port = return bundle = control

    float result_inner[TILE_LAYERS * GRID_ROWS * GRID_COLS];
    float temp_inner[(TILE_LAYERS + 2) * (GRID_ROWS)*GRID_COLS];
    float power_inner[TILE_LAYERS* GRID_ROWS * GRID_COLS];
    int i;

    ITER_LOOP:for ( i = 0; i < ITERATIONS ; i++)
        {
        int l;
        TILE_LOOP:for (l = 0; l < GRID_LAYERS; l+= TILE_LAYERS)
        {
            
            load( temp_inner,power_inner, tempIn, powerIn,  l);

            compute(power_inner, temp_inner, result_inner, l);
            
            store(tempOut, result_inner, l);
           
        }
        float *temp = tempIn;
        tempIn = tempOut;
        tempOut = temp; 
    } 
    return;
}
}