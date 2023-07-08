// OpenCL utility layer include
#include "xcl2.hpp"
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <math.h> 
#include <string.h>
#include "hotspot3D.h"
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

//Main computational kernel
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


                    tOut[c] = tIn[c]*cc + tIn[n]*cn + tIn[s]*cs + tIn[e]*ce + tIn[w]*cw + tIn[t]*ct + tIn[b]*cb + (dt/Cap) * pIn[c] + ct*AMB_TEMP;
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

int main (int argc, char** argv) {

   if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <XCLBIN File>" << std::endl;
        return EXIT_FAILURE;
    }

    std::string binaryFile = argv[1];
    cl_int err;
    cl::Context context;
    cl::Kernel krnl_3D;
    cl::CommandQueue q;
  

  /* Variable declaration/allocation. */
   char *pfile, *tfile, *ofile;// *testFile;

    pfile = "power_512x8";
    tfile = "temp_512x8";
    ofile = "output.out";

    float *powerIn, *tempOut, *tempIn, *tempOutHW;
  
    //Memory allocations
    powerIn = (float*)malloc(SIZE* sizeof(float));
    tempIn = (float*)malloc(SIZE*sizeof(float));
    tempOut = (float*)malloc(SIZE* sizeof(float));
    tempOutHW = (float*)malloc(SIZE* sizeof(float));
    
    std::vector<unsigned int, aligned_allocator<unsigned int>>powerIn_hw(SIZE);
    std::vector<unsigned int, aligned_allocator<unsigned int>>tempOut_hw(SIZE);
    std::vector<unsigned int, aligned_allocator<unsigned int>>tempIn_hw(SIZE);

 /* Initialize inputs. */
    readinput(powerIn,GRID_ROWS, GRID_COLS, GRID_LAYERS, pfile);
    readinput(tempIn, GRID_ROWS, GRID_COLS, GRID_LAYERS, tfile);                                    

  for(int i=0;i<SIZE;i++)
  {
    powerIn_hw[i]=powerIn[i];
    tempIn_hw[i]=tempIn[i];
  }
       
  //Get output results from software run
   computeTempCPU(powerIn, tempIn, tempOut);

  //Get output results from hardware run
  // OPENCL HOST CODE AREA START
    auto devices = xcl::get_xil_devices();
    // read_binary_file() is a utility API which will load the binaryFile
    // and will return the pointer to file buffer.
    auto fileBuf = xcl::read_binary_file(binaryFile);
    cl::Program::Binaries bins{{fileBuf.data(), fileBuf.size()}};
    bool valid_device = false;
    for (unsigned int i = 0; i < devices.size(); i++) {
        auto device = devices[i];
        // Creating Context and Command Queue for selected Device
        OCL_CHECK(err, context = cl::Context(device, nullptr, nullptr, nullptr, &err));
        OCL_CHECK(err, q = cl::CommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &err));
        std::cout << "Trying to program device[" << i << "]: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;
        cl::Program program(context, {device}, bins, nullptr, &err);
        if (err != CL_SUCCESS) {
            std::cout << "Failed to program device[" << i << "] with xclbin file!\n";
        } else {
            std::cout << "Device[" << i << "]: program successful!\n";
            OCL_CHECK(err, krnl_3D = cl::Kernel(program, "hotspot3D", &err));
            valid_device = true;
            break; // we break because we found a valid device
        }
    }
    if (!valid_device) {
        std::cout << "Failed to program any device found, exit!\n";
        exit(EXIT_FAILURE);
    }
        // void workload(float powerIn[GRID_ROWS * GRID_COLS * GRID_LAYERS],
        //       float tempIn[GRID_ROWS * GRID_COLS * GRID_LAYERS],
        //       float tempOut[GRID_ROWS * GRID_COLS * GRID_LAYERS]
        //       )

    // Allocate Buffer in Global Memory
    OCL_CHECK(err, cl::Buffer buffer_in1(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, SIZE,
                                         powerIn_hw.data(), &err));
    OCL_CHECK(err, cl::Buffer buffer_in2(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, SIZE,
                                         tempIn_hw.data(), &err));
    OCL_CHECK(err, cl::Buffer buffer_output(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, SIZE,
                                          tempOut_hw.data(), &err));


    OCL_CHECK(err, err = krnl_3D.setArg(0, buffer_in1));
    OCL_CHECK(err, err = krnl_3D.setArg(1, buffer_in2));
    OCL_CHECK(err, err = krnl_3D.setArg(2, buffer_output));
    //OCL_CHECK(err, err = krnl_3D.setArg(3, size));

    // Copy input data to device global memory
    OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_in1, buffer_in2,buffer_output}, 0 /* 0 means from host*/));

    // Launch the Kernel
    OCL_CHECK(err, err = q.enqueueTask(krnl_3D));
    q.finish();

    // Copy Result from Device Global Memory to Host Local Memory
    OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_output}, CL_MIGRATE_MEM_OBJECT_HOST));
    q.finish();

    // OPENCL HOST CODE AREA END

   
  for(int i=0;i<SIZE;i++)
  {
    tempOutHW[i]=tempOut_hw[i];
  }
    writeoutput(tempOutHW,GRID_ROWS, GRID_COLS, GRID_LAYERS, ofile);
    

   //Check results
   int has_errors = 0;
  for(int i = 0; i < SIZE; i++){
    if(tempOut_hw[i] != tempOut[i]){

      float tmp;
      if(tempOut_hw[i] > tempOut[i]){
        tmp = tempOut_hw[i] - tempOut[i];
      } else {
        tmp = tempOut[i] - tempOut_hw[i];
      }
      float error_rate = tmp /tempOut[i];
      if(error_rate > 0.005)
        has_errors++;
    }
  }


    if(has_errors)
       printf("\nTEST PASSED\n");
    else
        printf("\nTEST FAILED\n");
    
    //Free memory
    free(tempIn);
    free(tempOut);
    free(powerIn);

  return 0;
}
