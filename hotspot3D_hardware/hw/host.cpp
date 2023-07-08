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

void printArray(float *arr1, float *arr2 ,int len)
{
  for(int i=0;i<len;i++)
  {
   printf("\nseq[%d] = %f",i,arr1[i]);
   printf("\nhls[%d] = %f",i,arr2[i]);
   printf("\n###################");
  }
}

// HBM Pseudo-channel(PC) requirements
#define MAX_HBM_PC_COUNT 32
#define PC_NAME(n) n | XCL_MEM_TOPOLOGY
const int pc[MAX_HBM_PC_COUNT] = {
    PC_NAME(0),  PC_NAME(1),  PC_NAME(2),  PC_NAME(3),  PC_NAME(4),  PC_NAME(5),  PC_NAME(6),  PC_NAME(7),
    PC_NAME(8),  PC_NAME(9),  PC_NAME(10), PC_NAME(11), PC_NAME(12), PC_NAME(13), PC_NAME(14), PC_NAME(15),
    PC_NAME(16), PC_NAME(17), PC_NAME(18), PC_NAME(19), PC_NAME(20), PC_NAME(21), PC_NAME(22), PC_NAME(23),
    PC_NAME(24), PC_NAME(25), PC_NAME(26), PC_NAME(27), PC_NAME(28), PC_NAME(29), PC_NAME(30), PC_NAME(31)};

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

    pfile = "power_64x8";
    tfile = "temp_64x8";
    // pfile = "power_512x8";
    // tfile = "temp_512x8";
    ofile = "output.out";

    float *powerIn, *tempOut, *tempIn, *tempOutHW;
  
    //Memory allocations
    powerIn = (float*)malloc(SIZE* sizeof(float));
    tempIn = (float*)malloc(SIZE*sizeof(float));
    tempOut = (float*)malloc(SIZE* sizeof(float));
    tempOutHW = (float*)malloc(SIZE* sizeof(float));
    
    INTERFACE_WIDTH *wPowerIn = (INTERFACE_WIDTH *)malloc(SIZE/WIDTH_FACTOR*sizeof(INTERFACE_WIDTH));
    INTERFACE_WIDTH *wTempIn = (INTERFACE_WIDTH *)malloc(SIZE/WIDTH_FACTOR*sizeof(INTERFACE_WIDTH));
    INTERFACE_WIDTH *wTempOut = (INTERFACE_WIDTH *)malloc(SIZE/WIDTH_FACTOR*sizeof(INTERFACE_WIDTH));

    // std::vector<unsigned int, aligned_allocator<unsigned int>>powerIn_hw(SIZE);
    // std::vector<unsigned int, aligned_allocator<unsigned int>>tempOut_hw(SIZE);
    // std::vector<unsigned int, aligned_allocator<unsigned int>>tempIn_hw(SIZE);

    std::vector<INTERFACE_WIDTH, aligned_allocator<INTERFACE_WIDTH>>powerIn_hw(SIZE/WIDTH_FACTOR);
    std::vector<INTERFACE_WIDTH, aligned_allocator<INTERFACE_WIDTH>>tempOut_hw(SIZE/WIDTH_FACTOR);
    std::vector<INTERFACE_WIDTH, aligned_allocator<INTERFACE_WIDTH>>tempIn_hw(SIZE/WIDTH_FACTOR);



 /* Initialize inputs. */
    readinput(powerIn,GRID_ROWS, GRID_COLS, GRID_LAYERS, pfile);
    readinput(tempIn, GRID_ROWS, GRID_COLS, GRID_LAYERS, tfile);    

    readinputWide(wTempIn, tempIn, GRID_ROWS, GRID_COLS, GRID_LAYERS);
    readinputWide(wPowerIn, powerIn, GRID_ROWS, GRID_COLS, GRID_LAYERS);

  for(int i=0;i<SIZE/WIDTH_FACTOR;i++)
  {
    powerIn_hw[i]=wPowerIn[i];
    tempIn_hw[i]=wTempIn[i];
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

    // int i =1;

    // cl_mem_ext_ptr_t inBufExt1;
    // cl_mem_ext_ptr_t inBufExt2;
    // cl_mem_ext_ptr_t outBufExt;

    // inBufExt1.obj = powerIn_hw.data();
    // inBufExt1.param = 0;
    // inBufExt1.flags = pc[i * 4];

    // inBufExt2.obj = tempIn_hw.data();
    // inBufExt2.param = 0;
    // inBufExt2.flags = pc[(i * 4) + 1];

    // outBufExt.obj = tempOut_hw.data();
    // outBufExt.param = 0;
    // outBufExt.flags = pc[(i * 4) + 2];

    // Allocate Buffer in Global Memory
    // OCL_CHECK(err, cl::Buffer buffer_in1(context, CL_MEM_USE_HOST_PTR | CL_MEM_EXT_PTR_XILINX | CL_MEM_READ_ONLY, sizeof(INTERFACE_WIDTH) * (SIZE)/WIDTH_FACTOR,
    //                                     inBufExt1, &err));
    // OCL_CHECK(err, cl::Buffer buffer_in2(context, CL_MEM_USE_HOST_PTR | CL_MEM_EXT_PTR_XILINX | CL_MEM_READ_ONLY,sizeof(INTERFACE_WIDTH) * (SIZE)/WIDTH_FACTOR,
    //                                     inBufExt2, &err));
    // OCL_CHECK(err, cl::Buffer buffer_output(context, CL_MEM_USE_HOST_PTR | CL_MEM_EXT_PTR_XILINX | CL_MEM_READ_WRITE, sizeof(INTERFACE_WIDTH) * (SIZE)/WIDTH_FACTOR,
    //                                     outBufExt, &err));
      

    OCL_CHECK(err, cl::Buffer buffer_in1(context, CL_MEM_USE_HOST_PTR |  CL_MEM_READ_ONLY, sizeof(INTERFACE_WIDTH) * (SIZE)/WIDTH_FACTOR,
                                        powerIn_hw.data(), &err));
    OCL_CHECK(err, cl::Buffer buffer_in2(context, CL_MEM_USE_HOST_PTR |  CL_MEM_READ_ONLY,sizeof(INTERFACE_WIDTH) * (SIZE)/WIDTH_FACTOR,
                                        tempIn_hw.data(), &err));
    OCL_CHECK(err, cl::Buffer buffer_output(context, CL_MEM_USE_HOST_PTR |  CL_MEM_READ_WRITE, sizeof(INTERFACE_WIDTH) * (SIZE)/WIDTH_FACTOR,
                                        tempOut_hw.data(), &err));


    OCL_CHECK(err, err = krnl_3D.setArg(0, buffer_in1));
    OCL_CHECK(err, err = krnl_3D.setArg(1, buffer_in2));
    OCL_CHECK(err, err = krnl_3D.setArg(2, buffer_output));
    //OCL_CHECK(err, err = krnl_3D.setArg(3, size));

    // Copy input data to device global memory
    OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_in1, buffer_in2}, 0 /* 0 means from host*/));

    // Launch the Kernel
    OCL_CHECK(err, err = q.enqueueTask(krnl_3D));
    // q.finish();

    // Copy Result from Device Global Memory to Host Local Memory
    OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_output}, CL_MIGRATE_MEM_OBJECT_HOST));
    q.finish();

    // OPENCL HOST CODE AREA END

   
  for(int i=0;i<SIZE/WIDTH_FACTOR;i++)
  {
    wTempOut[i]=tempOut_hw[i];
  }

  writeinputWide(wTempOut, tempOutHW, GRID_ROWS, GRID_COLS, GRID_LAYERS);

  writeoutput(tempOutHW, GRID_ROWS, GRID_COLS, GRID_LAYERS, ofile);
    

   //Check results
   int has_errors = 0;
  for(int i = 0; i < SIZE; i++){
    if(tempOutHW[i] != tempOut[i]){

      float tmp;
      if(tempOutHW[i] > tempOut[i]){
        tmp = tempOutHW[i] - tempOut[i];
      } else {
        tmp = tempOut[i] - tempOutHW[i];
      }
      float error_rate = tmp /tempOut[i];
      if(error_rate > 0.005)
        has_errors++;
    }
  }


    if(has_errors)
       printf("\nTEST PASSED\n");
    else
        printf("\nTEST  FAILED\n");
    
    //Free memory
    free(wPowerIn);
    free(wTempIn);
    free(wTempOut);
    free(tempIn);
    free(tempOut);
    free(powerIn);

  return 0;
}
