

#include"hotspot3D.h"


float hotspot_stencil_core(float temp_center, float temp_top, float temp_bottom, float temp_west,float temp_east,float temp_north, float temp_south, float power_center ) {
    #pragma HLS inline off
 
    float result = temp_center * cc + temp_north * cn + temp_south * cs + temp_east * ce + temp_west * cw + temp_top * ct + temp_bottom * cb + AMB_TEMP * ct + power_center * (dt / Cap);

    return result;
}

void compute(int flag, float power[TILE_LAYERS * GRID_ROWS * GRID_COLS], float temp[(TILE_LAYERS + 2) * (GRID_ROWS)*GRID_COLS], float result[TILE_LAYERS * GRID_ROWS * GRID_COLS], int l)
{
    if(flag)
    {
    int r, i, j, k, ii;
    float temp_top[PARA_FACTOR], temp_bottom[PARA_FACTOR], temp_west[PARA_FACTOR], temp_east[PARA_FACTOR], 
          temp_center[PARA_FACTOR], power_center[PARA_FACTOR], temp_north[PARA_FACTOR], temp_south[PARA_FACTOR];

    float temp_rf_T [PARA_FACTOR][GRID_COLS* 2 / PARA_FACTOR + 1];
    #pragma HLS array_partition variable=temp_rf_T complete dim=0

    float temp_rf [PARA_FACTOR][GRID_COLS* 2 / PARA_FACTOR + 1];
    #pragma HLS array_partition variable=temp_rf complete dim=0

    float temp_rf_B [PARA_FACTOR][GRID_COLS* 2 / PARA_FACTOR + 1];
    #pragma HLS array_partition variable=temp_rf_B complete dim=0

    //Loop for copying first row 
    INIT_LOOP1_1:for ( i = 0 ; i < GRID_COLS/PARA_FACTOR; i++) {
        #pragma HLS pipeline II=1
        INIT_LOOP1_2:for (ii = 0; ii < PARA_FACTOR; ii++) {
            #pragma HLS unroll
            temp_rf[ii][i] = temp[GRID_COLS*GRID_ROWS + i*PARA_FACTOR + ii];            
        }
    }

    INTI_LOOP2_1:for (  j=0; i < GRID_COLS * 2 / PARA_FACTOR + 1; i++, j++) {
        #pragma HLS pipeline II=1
        INTI_LOOP2_2:for (ii = 0; ii < PARA_FACTOR; ii++) {
            #pragma HLS unroll
            temp_rf_T[ii][i] = temp[j*PARA_FACTOR + ii];
            temp_rf[ii][i] = temp[GRID_COLS*GRID_ROWS + j*PARA_FACTOR + ii];
            temp_rf_B[ii][i] = temp[2*GRID_COLS*GRID_ROWS + j*PARA_FACTOR + ii];            
        }
    }


    CALCULATION1:for (i = 0; i < GRID_COLS / PARA_FACTOR * GRID_ROWS  ; i++) {
        #pragma HLS pipeline II=1
        INNER_CALCULATION1:for (k = 0; k < PARA_FACTOR; k++) {
            #pragma HLS unroll
            temp_center[k] = temp_rf[k][GRID_COLS / PARA_FACTOR];
            temp_bottom[k] = (l==0)?temp_center[k]:temp_rf_T[k][GRID_COLS / PARA_FACTOR];
            temp_top[k] = (l==BOTTOM)?temp_center[k]: temp_rf_B[k][GRID_COLS / PARA_FACTOR];
            temp_north[k] = (i < GRID_COLS / PARA_FACTOR)? temp_center[k] : temp_rf[k][0];
            temp_south[k] = (i >=  GRID_COLS / PARA_FACTOR * (GRID_ROWS - 1))? temp_center[k] : temp_rf[k][GRID_COLS / PARA_FACTOR * 2];
            temp_west[k] = ((i % (GRID_COLS / PARA_FACTOR)) == 0 && k == 0) ? temp_center[k] : temp_rf[(k - 1 + PARA_FACTOR) % PARA_FACTOR][GRID_COLS / PARA_FACTOR - (k == 0) ];
            temp_east[k] = ((i % (GRID_COLS / PARA_FACTOR)) == (GRID_COLS / PARA_FACTOR - 1) && k == PARA_FACTOR - 1) ? temp_center[k] : temp_rf[(k + 1 + PARA_FACTOR) % PARA_FACTOR][GRID_COLS / PARA_FACTOR + (k == (PARA_FACTOR - 1)) ];           
            power_center[k] = power[i * PARA_FACTOR + k];
            result[i * PARA_FACTOR + k] = hotspot_stencil_core(temp_center[k], temp_top[k], temp_bottom[k], temp_west[k], temp_east[k], temp_north[k], temp_south[k], power_center[k]);
        }
        //Shifting by 1 column and adding new column
        INNER_CALCULATION2:for (k = 0; k < PARA_FACTOR; k++) {
            #pragma hls unroll
            for (j = 0; j < GRID_COLS * 2 / PARA_FACTOR; j++) {
                #pragma hls unroll
                temp_rf_T[k][j] = temp_rf_T[k][j + 1];
                temp_rf[k][j] = temp_rf[k][j + 1];
                temp_rf_B[k][j] = temp_rf_B[k][j + 1];
            }        
            temp_rf_T[k][GRID_COLS * 2 / PARA_FACTOR] = temp[ GRID_COLS * 2 + (i+1) * PARA_FACTOR + k - GRID_COLS];
            temp_rf[k][GRID_COLS * 2 / PARA_FACTOR] = temp[GRID_COLS*GRID_ROWS + GRID_COLS * 2 + (i+1 ) * PARA_FACTOR + k - GRID_COLS];
            temp_rf_B[k][GRID_COLS * 2 / PARA_FACTOR] = temp[2*GRID_COLS*GRID_ROWS + GRID_COLS * 2 + (i+1 ) * PARA_FACTOR + k  - GRID_COLS];
        }

  
       
    }
    }
}


void load(int flag, float *temp_inner,float *power_inner, INTERFACE_WIDTH *tempIn, INTERFACE_WIDTH *powerIn, int l)
{
    
    if(flag)
    {
    int z,y,x,i, j;
    uint32_t lowEnd, highEnd, temp;
    float f;
    if(l == 0)
    {
        LOAD1:for(i = 0; i < (TILE_LAYERS+1)*GRID_ROWS*GRID_COLS ; i+= WIDTH_FACTOR)
        {
        #pragma HLS pipeline II=1
        z = i/(GRID_ROWS*GRID_COLS) + 1;
        y = (i/GRID_COLS)%GRID_COLS;
        x = i%GRID_ROWS;
        lowEnd = 0;
        highEnd = 31;
        LOAD1_WIDTHFACTOR:for(j = 0 ; j < WIDTH_FACTOR ; j++)
        {
        temp = tempIn[i/WIDTH_FACTOR].range(highEnd, lowEnd);
        f = *(float*)&temp;
        temp_inner[z*GRID_ROWS*GRID_COLS + y*GRID_COLS + x + j] = f;
        lowEnd += 32;
        highEnd += 32;
        }
        }
    }
    else
    {
    int tempOffset = ((l-1)*GRID_ROWS*GRID_COLS)/WIDTH_FACTOR;

    LOAD2:for (i = 0; i < (TILE_LAYERS+2)*GRID_ROWS*GRID_COLS; i+= WIDTH_FACTOR)
    {
        #pragma HLS pipeline II=1
        z = i/(GRID_ROWS*GRID_COLS);
        y = (i/GRID_COLS)%GRID_COLS;
        x = i%GRID_ROWS;
        lowEnd = 0;
        highEnd = 31;
        for(j = 0 ; j < WIDTH_FACTOR ; j++)
        {
        temp = tempIn[tempOffset + i/WIDTH_FACTOR].range(highEnd, lowEnd);
        f = *(float*)&temp;
        temp_inner[z*GRID_ROWS*GRID_COLS + y*GRID_COLS + x + j] = f;
        lowEnd += 32;
        highEnd += 32;
        }
    }
    }

    int powerOffset = (l*GRID_ROWS*GRID_COLS)/WIDTH_FACTOR;
    LOAD3:for(i = 0; i < (TILE_LAYERS*GRID_ROWS*GRID_COLS); i+= WIDTH_FACTOR)
    {
        #pragma HLS pipeline II=1
        z = i/(GRID_ROWS*GRID_COLS);
        y = (i/GRID_COLS)%GRID_COLS;
        x = i%GRID_ROWS;
        lowEnd = 0;
        highEnd = 31;
        LAOD3_WIDTHFACTOR:for(j = 0 ; j < WIDTH_FACTOR ; j++)
        {
        temp = powerIn[powerOffset + i/WIDTH_FACTOR].range(highEnd, lowEnd);
        f = *(float*)&temp;
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
    int z,y,x, i, j;
    uint32_t lowEnd, highEnd, temp;
    float fc;
    int offset = (l * GRID_ROWS * GRID_COLS)/WIDTH_FACTOR;
    STORE1:for (i = 0; i < GRID_ROWS*GRID_COLS*TILE_LAYERS; i+= WIDTH_FACTOR)
    {
        #pragma HLS pipeline II=1
        z = i/(GRID_ROWS*GRID_COLS);
        y = (i/GRID_COLS)%GRID_COLS;
        x = i%GRID_ROWS;
        lowEnd = 0;
        highEnd = 31;
        STORE1_WIDTHFACTOR:for(j = 0 ; j < WIDTH_FACTOR ; j++)
        {
        fc = result_inner[z*GRID_ROWS*GRID_COLS + y*GRID_COLS + x + j];
        temp = *(uint32_t*)&fc;
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
    #pragma HLS INTERFACE m_axi port=tempIn offset=slave bundle=gmem max_write_burst_length=DWIDTH max_read_burst_length=DWIDTH
    #pragma HLS INTERFACE s_axilite port=tempIn bundle=control
    #pragma HLS INTERFACE m_axi port=powerIn offset=slave bundle=gmem max_write_burst_length=DWIDTH max_read_burst_length=DWIDTH
    #pragma HLS INTERFACE s_axilite port=powerIn bundle=control
    #pragma HLS INTERFACE m_axi port=tempOut offset=slave bundle=gmem max_write_burst_length=DWIDTH max_read_burst_length=DWIDTH
    #pragma HLS INTERFACE s_axilite port=tempOut bundle=control

    #pragma HLS INTERFACE s_axilite port=return bundle=control

    #pragma HLS DATAFLOW

    #pragma HLS stable variable=powerIn

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

    ITERATION_LOOP:for ( i = 0; i < ITERATIONS ; i++)
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
