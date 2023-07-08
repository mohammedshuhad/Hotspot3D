#include"3D.h"

void hotspot3D(float pIn[TILE_LAYERS * GRID_ROWS * GRID_COLS], float tIn[(TILE_LAYERS + 2) * (GRID_ROWS)*GRID_COLS], float tOut[TILE_LAYERS * GRID_ROWS * GRID_COLS], int l)
{
    int  w, e, n, s, b, t;
    int z,r,c;

    int counter = 0;

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
void load(float *temp_inner,float *power_inner, INTERFACE_WIDTH *tempIn, INTERFACE_WIDTH *powerIn, int l)
{
    int powerOffset = (l*GRID_ROWS*GRID_COLS)/WIDTH_FACTOR;
    if(l == 0)
    {
        // for(int i = 0 ; i < GRID_ROWS*GRID_COLS; i+= WIDTH_FACTOR)
        // {
        // int z = i/(GRID_ROWS*GRID_COLS);
        // int y = (i/GRID_COLS)%GRID_COLS;
        // int x = i%GRID_ROWS;
        // for(int j = 0 ; j < WIDTH_FACTOR ; j++)
        // {
        // temp_inner[z*GRID_ROWS*GRID_COLS + y*GRID_COLS + x + j] = 0;
        // }
        // }
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

//   memcpy(temp_inner, tempIn + ((l-1) * GRID_ROWS * GRID_COLS), sizeof(float) * (TILE_LAYERS+2) * (GRID_ROWS)*GRID_COLS);
//   memcpy(power_inner, powerIn + (l * GRID_ROWS * GRID_COLS), sizeof(float) * TILE_LAYERS * GRID_ROWS * GRID_COLS);
}

void store(INTERFACE_WIDTH *tempOut, float *result_inner, int l)
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
//   memcpy(tempOut + l * GRID_ROWS * GRID_COLS, result_inner, sizeof(float) * TILE_LAYERS * GRID_ROWS * GRID_COLS);
}

extern "C" {
void workload(INTERFACE_WIDTH powerIn[GRID_ROWS * GRID_COLS * GRID_LAYERS],
              INTERFACE_WIDTH tempIn[GRID_ROWS * GRID_COLS * GRID_LAYERS],
              INTERFACE_WIDTH tempOut[GRID_ROWS * GRID_COLS * GRID_LAYERS]
              )
{

#pragma HLS INTERFACE m_axi port = tempIn offset = slave bundle = gmem
#pragma HLS INTERFACE m_axi port = powerIn offset = slave bundle = gmem
#pragma HLS INTERFACE m_axi port = tempOut offset = slave bundle = gmem

#pragma HLS INTERFACE s_axilite port = tempIn bundle = control
#pragma HLS INTERFACE s_asizexilite port = powerIn bundle = control
#pragma HLS INTERFACE s_axilite port = tempOut bundle = control

#pragma HLS INTERFACE s_axilite port = return bundle = control

    float result_inner[TILE_LAYERS * GRID_ROWS * GRID_COLS];
    float temp_inner[(TILE_LAYERS + 2) * (GRID_ROWS)*GRID_COLS];
    float power_inner[TILE_LAYERS* GRID_ROWS * GRID_COLS];
    int i, l;

    ITER_LOOP:for ( i = 0; i < ITERATIONS ; i++)
        {
        TILE_LOOP:for (l = 0; l < GRID_LAYERS; l+= TILE_LAYERS)
        {
            load( temp_inner,power_inner, tempIn, powerIn, l);

            hotspot3D(power_inner, temp_inner, result_inner, l);
            
            store(tempOut, result_inner, l);
        }
        INTERFACE_WIDTH *temp = tempIn;
        tempIn = tempOut;
        tempOut = temp; 
    } 
    return;
}
}