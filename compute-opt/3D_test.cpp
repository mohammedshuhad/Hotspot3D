#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <stdlib.h> 
#include <math.h> 
#include <sys/time.h>
#include <string.h>
#include<iostream>
#include "3D.h"

int check_data( float *vdata, float *vref ) {

  int has_errors = 0;
  for(int i = 0; i < GRID_COLS * GRID_ROWS * GRID_LAYERS; i++){
    if(vdata[i] != vref[i]){

      float tmp;
      //std::cout<< vdata[i] << "," << vref[i] << "\n";
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
    for (i=0; i <= grid_layers-1 ; i++) 
      for (j=0; j <= grid_rows-1; j++)
        for (k=0; k <= grid_cols-1; k++)
          {
            if (fgets(str, STR_SIZE, fp) == NULL) fatal("Error reading file\n");
            if (feof(fp))
            {
              printf("i = %d, j = %d, k = %d", i, j, k);
              fatal("not enough lines in file");
            }
            if ((sscanf(str, "%f", &val) != 1))
              fatal("invalid file format");
            vect[i*grid_cols*grid_rows + j*grid_cols + k] = val;
          }

    fclose(fp);	

}


void writeoutput(float *vect, int grid_rows, int grid_cols, int grid_layers, char *file) {

    int i,j,k, index=0;
    FILE *fp;
    char str[STR_SIZE];

    fp = fopen("output.out", "w+" );
    if( fp == NULL )
      printf( "The file was not opened\n" );
    for (i=0; i <= grid_layers-1 ; i++) 
      for (j=0; j <= grid_rows-1; j++)
        for (k=0; k <= grid_cols-1; k++)
          {
            sprintf(str, "%d\t%g\n", index, vect[i*grid_cols*grid_rows + j*grid_cols + k]);
            fputs(str,fp);
            index++;
          }

    fclose(fp);	
}

void computeTempCPU(float *pIn, float* tIn, float *tOut) 
{  
    int c,w,e,n,s,b,t;
    int x,y,z;
    int i = 0;
    do{
        for(z = 0; z < GRID_LAYERS; z++)
            for(y = 0; y < GRID_ROWS; y++)
                for(x = 0; x < GRID_COLS; x++)
                {
                    c = x + y * GRID_COLS + z * GRID_COLS * GRID_ROWS;
                    w = (x == 0) ? c      : c - 1;
                    e = (x == GRID_COLS - 1) ? c : c + 1;
                    n = (y == 0) ? c      : c - GRID_COLS;
                    s = (y == GRID_ROWS - 1) ? c : c + GRID_COLS;
                    b = (z == 0) ? c      : c - GRID_COLS * GRID_ROWS;
                    t = (z == GRID_LAYERS - 1) ? c : c + GRID_COLS * GRID_ROWS; 


                    tOut[c] = tIn[c]*cc + tIn[n]*cn + tIn[s]*cs + tIn[e]*ce + tIn[w]*cw + tIn[t]*ct + tIn[b]*cb + (stepDivCap) * pIn[c] + ct*AMB_TEMP;
                }
        float *temp = tIn;
        tIn = tOut;
        tOut = temp; 
        i++;
    }
    while(i < ITERATIONS);

}

void printArray(float *arr1, float *arr2 ,int len)
{
  for(int i=0;i<len;i++)
  {
   printf("\nseq[%d] = %f",i,arr1[i]);
   printf("\nhls[%d] = %f",i,arr2[i]);
   printf("\n###################");
  }
}

int main()
{

//./3D 512 8 100 ../../data/hotspot3D/power_512x8 ../../data/hotspot3D/temp_512x8 output.out 20
// 0   1   2 3   4                                5                               6          7
    
    char *pfile, *tfile, *ofile1, *ofile2;// *testFile;
    ofile1= "output.out";
    ofile2 = "orig.out";
    pfile = "power_512x8";
    tfile = "temp_512x8";
    // ofile = "output.out";
    float *powerIn, *tempOut, *tempIn, *tempCopy;

    //Memory allocations
    powerIn = (float*)malloc(size* sizeof(float));
    tempCopy = (float*)malloc(size * sizeof(float));
    tempIn = (float*)malloc(size*sizeof(float));
    tempOut = (float*)malloc(size* sizeof(float));
    float* answer = (float*)malloc(size*sizeof(float));
    
    readinput(powerIn,GRID_ROWS, GRID_COLS, GRID_LAYERS,  pfile);
    readinput(tempIn, GRID_ROWS, GRID_COLS, GRID_LAYERS, tfile);                                    
    memcpy(tempCopy,tempIn, size * sizeof(float));

    struct timeval start, stop;
    float time_FPGA,time_CPU;

    gettimeofday(&start,NULL);
    workload(powerIn, tempIn, tempOut);
    gettimeofday(&stop,NULL);
    time_FPGA     = (stop.tv_usec-start.tv_usec)*1.0e-6 + stop.tv_sec - start.tv_sec;

    gettimeofday(&start,NULL);
    computeTempCPU(powerIn, tempCopy, answer);
    gettimeofday(&stop,NULL);
    time_CPU = (stop.tv_usec-start.tv_usec)*1.0e-6 + stop.tv_sec - start.tv_sec;

    float acc = accuracy(tempOut,answer,GRID_ROWS*GRID_COLS*GRID_LAYERS);
    float speedup = time_CPU / time_FPGA;

    printf("Time for sequential   run : %.3f (s)\n",time_CPU);
    printf("Time for parallelized run : %.3f (s)\n",time_FPGA);
    printf("Accuracy: %e\n",acc);
    printf("Speedup = %.3f \n",speedup);
    writeoutput(tempOut,GRID_ROWS, GRID_COLS, GRID_LAYERS, ofile1);
    writeoutput(answer,GRID_ROWS, GRID_COLS, GRID_LAYERS, ofile2);
    
    if(check_data( tempOut,answer ))
       printf("\nTEST PASSED\n");
    else
        printf("\nTEST FAILED\n");
    
    //Free memory
    free(tempIn);
    free(tempOut);
    free(powerIn);
    
    return 0;

}