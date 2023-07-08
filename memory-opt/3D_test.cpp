#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <stdlib.h> 
#include <math.h> 
#include <sys/time.h>
#include <string.h>
#include "3D.h"

int check_data( float *vdata, float *vref ) {

  int has_errors = 0;
  for(int i = 0; i < GRID_COLS * GRID_ROWS * GRID_LAYERS; i++){
    if(vdata[i] != vref[i]){

      float tmp;
      if(vdata[i] > vref[i]){
        tmp = vdata[i] - vref[i];
      } else {
        tmp = vref[i] - vdata[i];
      }
      float error_rate = tmp /vref[i];
      if(error_rate > 0.005)
        has_errors++;
    }
  }
  // Return true if it's correct.
  return !has_errors;
}

void fatal(char *s)
{
    fprintf(stderr, "Error: %s\n", s);
}
float accuracy(float *arr1, float *arr2, int len)
{
    float err = 0.0; 
    int i;
    for(i = 0; i < len; i++)
    {
        err += (arr1[i]-arr2[i]) * (arr1[i]-arr2[i]);
    }

    return (float)sqrt(err/len);


}
void readinput(float *vect, int grid_rows, int grid_cols, int grid_layers, char *file) {
    int i,j,k;
    FILE *fp;
    char str[STR_SIZE];
    float val;
    
    if( (fp  = fopen(file, "r" )) ==0 )
      fatal( "The file was not opened" );
    for (i=0; i <= grid_rows-1; i++) 
      for (j=0; j <= grid_cols-1; j++)
        for (k=0; k <= grid_layers-1; k++)
          {
            if (fgets(str, STR_SIZE, fp) == NULL) fatal("Error reading file\n");
            if (feof(fp))
              fatal("not enough lines in file");
            if ((sscanf(str, "%f", &val) != 1))
              fatal("invalid file format");
            vect[i*grid_cols+j+k*grid_rows*grid_cols] = val;
          }

    fclose(fp);	

}


void writeinputWide(INTERFACE_WIDTH *src, float *dest, int grid_rows, int grid_cols, int grid_layers) 
{
  for (uint32_t i = 0; i < grid_rows*grid_cols*grid_layers; i+= WIDTH_FACTOR)
  {
    int z = i/(grid_rows*grid_cols);
    int y = (i/grid_cols)%grid_cols;
    int x = i%grid_rows;
    uint32_t lowEnd = 0;
    uint32_t highEnd = 31;
    for(int j = 0 ; j < WIDTH_FACTOR ; j++)
    {
      uint32_t temp = src[i/WIDTH_FACTOR].range(highEnd, lowEnd);
      float f = *(float*)&temp;
      dest[z*GRID_COLS*GRID_ROWS + y*GRID_COLS + x + j] = f;
      lowEnd += 32;
      highEnd += 32;
    }
  }
}


void writeoutput(float *vect, int grid_rows, int grid_cols, int grid_layers, char *file) {

    int i,j,k, index=0;
    FILE *fp;
    char str[STR_SIZE];

    fp = fopen("output.out", "w+" );
    if( fp == NULL )
      printf( "The file was not opened\n" );

    for (i=0; i < grid_rows; i++) 
      for (j=0; j < grid_cols; j++)
        for (k=0; k < grid_layers; k++)
          {
            sprintf(str, "%d\t%g\n", index, vect[i*grid_cols+j+k*grid_rows*grid_cols]);
            fputs(str,fp);
            index++;
          }

    fclose(fp);	
}

void computeTempCPU(float *pIn, float* tIn, float *tOut, 
        int nx, int ny, int nz, int numiter) 
{  
    int c,w,e,n,s,b,t;
    int x,y,z;
    int i = 0;
    do{
        for(z = 0; z < nz; z++)
            for(y = 0; y < ny; y++)
                for(x = 0; x < nx; x++)
                {
                    c = x + y * nx + z * nx * ny;
                    w = (x == 0) ? c      : c - 1;
                    e = (x == nx - 1) ? c : c + 1;
                    n = (y == 0) ? c      : c - nx;
                    s = (y == ny - 1) ? c : c + nx;
                    b = (z == 0) ? c      : c - nx * ny;
                    t = (z == nz - 1) ? c : c + nx * ny; 


                    tOut[c] = tIn[c]*cc + tIn[n]*cn + tIn[s]*cs + tIn[e]*ce + tIn[w]*cw + tIn[t]*ct + tIn[b]*cb + (dt/Cap) * pIn[c] + ct*AMB_TEMP;
                }
        float *temp = tIn;
        tIn = tOut;
        tOut = temp; 
        i++;
    }
    while(i < ITERATIONS);

}

void wprint_array(INTERFACE_WIDTH* C)
{
  int counter = 0;
  printf("\nPrinting Array\n");
  for (uint32_t i = 0; i < size; i+= WIDTH_FACTOR)
  {
    uint32_t lowEnd = 0;
    uint32_t highEnd = 31;
    for(int j = 0 ; j < WIDTH_FACTOR ; j++)
    {
      uint32_t temp = C[i/WIDTH_FACTOR].range(highEnd, lowEnd);
      float f = *(float*)&temp;
      printf("%d : [%f]\n",counter++, f);
      lowEnd += 32;
      highEnd += 32;
    }
  }
}

void printArray(float *arr1, float *arr2 ,int len)
{
  for(int i=0;i<100;i++)
  {
   printf("\nseq[%d] = %f",i,arr1[i]);
   printf("\nhls[%d] = %f",i,arr2[i]);
   printf("\n###################");
  }
}

void readinputWide(INTERFACE_WIDTH *dest, float *src, int grid_rows, int grid_cols, int grid_layers) 
{
  for (uint32_t i = 0; i < grid_rows*grid_cols*grid_layers; i+= WIDTH_FACTOR)
  {
    int z = i/(grid_rows*grid_cols);
    int y = (i/grid_cols)%grid_cols;
    int x = i%grid_cols;
    uint32_t lowEnd = 0;
    uint32_t highEnd = 31;
    for(int j = 0 ; j < WIDTH_FACTOR ; j++)
    {
      float fc = src[z*grid_rows*grid_cols + y*grid_cols + x + j];
      uint32_t temp = *(uint32_t*)&fc;
      dest[i/WIDTH_FACTOR].range(highEnd, lowEnd) = temp;
      lowEnd += 32;
      highEnd += 32;
    }
  }
}

int main()
{
   
    char *pfile, *tfile, *ofile;// *testFile;

    pfile = "power_512x8";
    tfile = "temp_512x8";

    // pfile = "debug_power_10x4";
    // tfile = "debug_temp_10x4";

    ofile = "output.out";
    float *powerIn, *tempOut, *tempIn, *tempCopy;

    //Memory allocations
    powerIn = (float*)malloc(size* sizeof(float));
    tempCopy = (float*)malloc(size * sizeof(float));
    tempIn = (float*)malloc(size*sizeof(float));
    tempOut = (float*)malloc(size* sizeof(float));
    float* answer = (float*)malloc(size*sizeof(float));

    INTERFACE_WIDTH *wPowerIn = (INTERFACE_WIDTH *)malloc(size/WIDTH_FACTOR*sizeof(INTERFACE_WIDTH));
    INTERFACE_WIDTH *wTempIn = (INTERFACE_WIDTH *)malloc(size/WIDTH_FACTOR*sizeof(INTERFACE_WIDTH));
    INTERFACE_WIDTH *wTempOut = (INTERFACE_WIDTH *)malloc(size/WIDTH_FACTOR*sizeof(INTERFACE_WIDTH));
    
    // printf("Allocation OK\n");

    readinput(powerIn,GRID_ROWS, GRID_COLS, GRID_LAYERS,  pfile);
    readinput(tempIn, GRID_ROWS, GRID_COLS, GRID_LAYERS, tfile);  

    readinputWide(wTempIn, tempIn, GRID_ROWS, GRID_COLS, GRID_LAYERS);

    readinputWide(wPowerIn, powerIn, GRID_ROWS, GRID_COLS, GRID_LAYERS);
    
    // wprint_array(wPowerIn);
                                     
    memcpy(tempCopy,tempIn, size * sizeof(float));

    struct timeval start, stop;
    float time_FPGA,time_CPU;

    gettimeofday(&start,NULL);

    workload(wPowerIn, wTempIn, wTempOut);

    writeinputWide(wTempOut, tempOut, GRID_ROWS, GRID_COLS, GRID_LAYERS);

    // wprint_array(wTempOut);

    gettimeofday(&stop,NULL);
    time_FPGA     = (stop.tv_usec-start.tv_usec)*1.0e-6 + stop.tv_sec - start.tv_sec;

    gettimeofday(&start,NULL);
    computeTempCPU(powerIn, tempCopy, answer, GRID_COLS, GRID_ROWS, GRID_LAYERS, ITERATIONS);
    
    // printArray(answer, tempOut, size);
    gettimeofday(&stop,NULL);
    time_CPU = (stop.tv_usec-start.tv_usec)*1.0e-6 + stop.tv_sec - start.tv_sec;

    float acc = accuracy(tempOut,answer,GRID_ROWS*GRID_COLS*GRID_LAYERS);
    float speedup = time_CPU / time_FPGA;

    printf("Time for sequential   run : %.3f (s)\n",time_CPU);
    printf("Time for parallelized run : %.3f (s)\n",time_FPGA);
    printf("Accuracy: %e\n",acc);
    printf("Speedup = %.3f \n",speedup);
    writeoutput(tempOut,GRID_ROWS, GRID_COLS, GRID_LAYERS, ofile);
    
    if(check_data( tempOut,answer ))
       printf("\nTEST PASSED\n");
    else
        printf("\nTEST FAILED\n");
    
    //Free memory
    free(wPowerIn);
    free(wTempOut);
    free(wTempIn);
    free(tempIn);
    free(tempOut);
    free(powerIn);
    
    return 0;

}