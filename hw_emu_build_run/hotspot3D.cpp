#include "hotspot3D.h"

void compute(int flag, float pIn[TILE_LAYERS * GRID_ROWS * GRID_COLS], float tIn[(TILE_LAYERS + 2) * (GRID_ROWS)*GRID_COLS], float tOut[TILE_LAYERS * GRID_ROWS * GRID_COLS], int l)
{
    if(flag)
    {
    int  w, e, n, s, b, t;
    int z,r,c;

    Z_LOOP:for (  z = 0 ;  z < TILE_LAYERS ; z++)
        R_LOOP:for ( r = 0; r < GRID_ROWS; r++)
            C_LOOP:for ( c = 0; c < GRID_COLS; c++) 
            {
                
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
}
void load(int flag, float *temp_inner,float *power_inner, INTERFACE_WIDTH *tempIn, INTERFACE_WIDTH *powerIn, int l)
{
    if(flag)
    {
    int powerOffset = (l*GRID_ROWS*GRID_COLS)/WIDTH_FACTOR;
    if(l == 0)
    {
        for(int i = 0; i < (TILE_LAYERS+1)*GRID_ROWS*GRID_COLS ; i+= WIDTH_FACTOR)
        {
        int z = i/(GRID_ROWS*GRID_COLS) + 1;
        int y = (i/GRID_COLS)%GRID_COLS;
        int x = i%GRID_ROWS;
        uint32_t lowEnd = 0;
        uint32_t highEnd = 31;
        for(int j = 0 ; j < WIDTH_FACTOR ; j++)
        {
        uint32_t temp = tempIn[i/WIDTH_FACTOR].range(highEnd, lowEnd);
        float f = *(float*)&temp;
        temp_inner[z*GRID_ROWS*GRID_COLS + y*GRID_COLS + x + j] = f;
        lowEnd += 32;
        highEnd += 32;
        }
        }
    }
    else
    {
    int tempOffset = ((l-1)*GRID_ROWS*GRID_COLS)/WIDTH_FACTOR;

    for (uint32_t i = 0; i < (TILE_LAYERS+2)*GRID_ROWS*GRID_COLS; i+= WIDTH_FACTOR)
    {
        int z = i/(GRID_ROWS*GRID_COLS);
        int y = (i/GRID_COLS)%GRID_COLS;
        int x = i%GRID_ROWS;
        uint32_t lowEnd = 0;
        uint32_t highEnd = 31;
        for(int j = 0 ; j < WIDTH_FACTOR ; j++)
        {
        uint32_t temp = tempIn[tempOffset + i/WIDTH_FACTOR].range(highEnd, lowEnd);
        float f = *(float*)&temp;
        temp_inner[z*GRID_ROWS*GRID_COLS + y*GRID_COLS + x + j] = f;
        lowEnd += 32;
        highEnd += 32;
        }
    }
    }

    for(uint32_t i = 0; i < (TILE_LAYERS*GRID_ROWS*GRID_COLS); i+= WIDTH_FACTOR)
    {
        int z = i/(GRID_ROWS*GRID_COLS);
        int y = (i/GRID_COLS)%GRID_COLS;
        int x = i%GRID_ROWS;
        uint32_t lowEnd = 0;
        uint32_t highEnd = 31;
        for(int j = 0 ; j < WIDTH_FACTOR ; j++)
        {
        uint32_t temp = powerIn[powerOffset + i/WIDTH_FACTOR].range(highEnd, lowEnd);
        float f = *(float*)&temp;
        power_inner[z*GRID_ROWS*GRID_COLS + y*GRID_COLS + x + j] = f;
        lowEnd += 32;
        highEnd += 32;
        }
    }
    }
}

void store(int flag, INTERFACE_WIDTH *tempOut, float *result_inner, int l)
{
    if(flag)
    {
    int offset = (l * GRID_ROWS * GRID_COLS)/WIDTH_FACTOR;
    for (uint32_t i = 0; i < GRID_ROWS*GRID_COLS*TILE_LAYERS; i+= WIDTH_FACTOR)
    {
        int z = i/(GRID_ROWS*GRID_COLS);
        int y = (i/GRID_COLS)%GRID_COLS;
        int x = i%GRID_ROWS;
        uint32_t lowEnd = 0;
        uint32_t highEnd = 31;
        for(int j = 0 ; j < WIDTH_FACTOR ; j++)
        {
        float fc = result_inner[z*GRID_ROWS*GRID_COLS + y*GRID_COLS + x + j];
        uint32_t temp = *(uint32_t*)&fc;
        tempOut[offset + i/WIDTH_FACTOR].range(highEnd, lowEnd) = temp;
        lowEnd += 32;
        highEnd += 32;
        }
    }
    }
}

extern "C" {
void hotspot3D(INTERFACE_WIDTH powerIn[GRID_ROWS * GRID_COLS * GRID_LAYERS],
              INTERFACE_WIDTH tempIn[GRID_ROWS * GRID_COLS * GRID_LAYERS],
              INTERFACE_WIDTH tempOut[GRID_ROWS * GRID_COLS * GRID_LAYERS]
              )
              {
#pragma HLS INTERFACE m_axi port = tempIn offset = slave bundle = gmem
#pragma HLS INTERFACE m_axi port = powerIn offset = slave bundle = gmem
#pragma HLS INTERFACE m_axi port = tempOut offset = slave bundle = gmem

#pragma HLS INTERFACE s_axilite port = tempIn bundle = control
#pragma HLS INTERFACE s_axilite port = powerIn bundle = control
#pragma HLS INTERFACE s_axilite port = tempOut bundle = control

#pragma HLS INTERFACE s_axilite port = return bundle = control

    float result_inner_0[TILE_LAYERS * GRID_ROWS * GRID_COLS];
    #pragma HLS bind_storage variable=result_inner_0 type=RAM_2P impl = BRAM
    float temp_inner_0[(TILE_LAYERS + 2) * (GRID_ROWS)*GRID_COLS];
    #pragma HLS bind_storage variable=temp_inner_0 type=RAM_2P impl = BRAM
    float power_inner_0[TILE_LAYERS* GRID_ROWS * GRID_COLS];
    #pragma HLS bind_storage variable=power_inner_0 type=RAM_2P impl = BRAM

    float result_inner_1[TILE_LAYERS * GRID_ROWS * GRID_COLS];
    #pragma HLS bind_storage variable=result_inner_1 type=RAM_2P impl = BRAM
    float temp_inner_1[(TILE_LAYERS + 2) * (GRID_ROWS)*GRID_COLS];
    #pragma HLS bind_storage variable=temp_inner_1 type=RAM_2P impl = BRAM
    float power_inner_1[TILE_LAYERS* GRID_ROWS * GRID_COLS];
    #pragma HLS bind_storage variable=power_inner_1 type=RAM_2P impl = BRAM

    int i,l;

    ITER_LOOP:for ( i = 0; i < ITERATIONS ; i++)
        {
        TILE_LOOP:for (l = 0; l < GRID_LAYERS + 2; l+= TILE_LAYERS)
        {
            int load_flag = l >= 0 && l < GRID_LAYERS / TILE_LAYERS;
            int compute_flag = l >= 1 && l < GRID_LAYERS / TILE_LAYERS + 1;
            int store_flag = l >= 2 && l < GRID_LAYERS / TILE_LAYERS + 2;

            if(l % 2 == 0) 
            {
              load( load_flag, temp_inner_0,power_inner_0, tempIn, powerIn, l);
              compute(compute_flag, power_inner_1, temp_inner_1, result_inner_1, l - 1);
              store(store_flag, tempOut, result_inner_0, l - 2);
            }
            else if (l % 2 == 1) 
            {
              load(load_flag, temp_inner_1,power_inner_1, tempIn, powerIn, l );
              compute(compute_flag, power_inner_0, temp_inner_0, result_inner_0, l - 1);
              store(store_flag, tempOut, result_inner_1, l - 2);
            }
        }
        INTERFACE_WIDTH *temp = tempIn;
        tempIn = tempOut;
        tempOut = temp; 
    } 
    return;
}
}