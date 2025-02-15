/*! @example 
 @brief A program that implements data parallel variant\n
 we run the example as ./dataparallel <veclength> <TAOwidth> <0:static, 1:dynamic> \n
 where
 \param veclength  := length of vector\n
 \param TAOwidth := number of workders\n
 \param internal scheduling := 0:static or 1:dynamic\n
 \param TAO block length := the block length pertaining to the internal TAO \n
 \param PTT training attempts:= the number of iterations for multiparallel region\n
*/
#include "xitao.h"
#include <iostream>
#include <chrono>
#include <cstring>
#include <omp.h>
using namespace xitao;
#define TEST_XITAO_BASIC_DATAPARALLEL 0
int main(int argc, char *argv[]) {  
  if(argc < 7) {
    std::cout << "./dataparallel <veclength> <workers> <0:static, 1:dynamic> <TAO block length> <TAO initial width> <PTT training attempts>" << std::endl; 
    return 0;
  }  
  int i = 0;                                        
  int end = 10;
  // problem size default is 1000
  int N       = (argc > 1) ? atoi(argv[1]) : 1000; 
  // parallelism default is 4
  int workers = (argc > 2) ? atoi(argv[2]) : 4;    
  // static or dynamic scheduling default is static
  int sched   = (argc > 3) ? atoi(argv[3]) : 0;      
  // the fine grain TAO block size (in case of multiparallel SPMD region)
  int block_length   = (argc > 4) ? atoi(argv[4]) : 0;      
  // the fine grain TAO block size (in case of multiparallel SPMD region)
  int tao_width   = (argc > 5) ? atoi(argv[5]) : 0;        
  // the number of iterations for multiparallel region
  int iter_count     = (argc > 6) ? atoi(argv[6]) : 1;      
  std::cout << "N: " << N << std::endl;           
  std::cout << "P: " << workers << std::endl;
  std::cout << "Block: " << block_length << std::endl;
  // Set an OMP scheduling that matches that of XiTAO 
  if(sched == xitao_vec_static) {
    omp_set_schedule(omp_sched_t::omp_sched_static, block_length);
    std::cout << "Vector region scheduling: static" << std::endl;
  }
  else {
    omp_set_schedule(omp_sched_t::omp_sched_dynamic, block_length);
    std::cout << "Vector region scheduling: dynamic" << std::endl;
  }
  // Set threads to match those of the XiTAO parallel region
  omp_set_num_threads(workers);  
  int **A, **B, **C;
  A = new int*[N];
  B = new int*[N];
  C = new int*[N];
  // Explicity touch the memory in the master thread to avoid any allocation happening at the thread level (e.g., NUMA first touch)
  // This is an effort to make the comparison as fair as possible by removing any streaming effect of the matmul
  for(int r = 0 ; r < N; ++r) {
    A[r] = new int[N];
    std::memset(A[r], 0, sizeof(int) * N);
    B[r] = new int[N];
    std::memset(B[r], 0, sizeof(int) * N);
    C[r] = new int[N];
    std::memset(C[r], 0, sizeof(int) * N);
  }

  std::chrono::time_point<std::chrono::system_clock> start_time, end_time;
  start_time = std::chrono::system_clock::now();
  for(int i = 0; i < N; ++i)
    for (int j = 0; j < N; ++j) 
    { 
      C[i][i] = 0; 
      for (int k = 0; k < N; ++k) 
        C[i][j] += A[i][k] *  
                   B[k][j]; 
    }  
  end_time = std::chrono::system_clock::now();  
  std::chrono::duration<double> elapsed_seconds = end_time - start_time;
  std::cout << "Total time serial: " << elapsed_seconds.count()  << std::endl;
#if TEST_XITAO_BASIC_DATAPARALLEL
  // Init XiTAO with workers 
  gotao_init_hw(workers, -1 , -1);
  // Start the worker threads
  gotao_start();  
  start_time = std::chrono::system_clock::now();
  __xitao_vec_region(tao_width, i, N, sched, 
    for (int j = 0; j < N; j++) 
     { 
       C[i][i] = 0; 
       for (int k = 0; k < N; k++) 
         C[i][j] += A[i][k] *  
                    B[k][j]; 
     }
  );
  gotao_fini();
  end_time = std::chrono::system_clock::now();  
  elapsed_seconds = end_time - start_time;
  std::cout << "Total time XiTAO: " << elapsed_seconds.count()  << std::endl;
  std::cout << "Total successful steals: " << tao_total_steals << std::endl;  
#endif  
  // Start the worker threads
  for (int iter = 0; iter < iter_count; ++iter) {
    // invalidate the caches 
    for(int r = 0 ; r < N; ++r) {
      A[r] = new int[N];
      std::memset(A[r], rand(), sizeof(int) * N);
      B[r] = new int[N];
      std::memset(B[r], rand(), sizeof(int) * N);
      C[r] = new int[N];
      std::memset(C[r], rand(), sizeof(int) * N);
    }
    // Init XiTAO with workers 
    gotao_init_hw(workers, -1 , -1);
    start_time = std::chrono::system_clock::now();
    __xitao_vec_multiparallel_region(tao_width, i, N, sched, block_length, 
      for (int j = 0; j < N; j++) 
       { 
         C[i][i] = 0; 
         for (int k = 0; k < N; k++) 
           C[i][j] += A[i][k] *  
                      B[k][j]; 
       }
    );
    gotao_fini();
    end_time = std::chrono::system_clock::now();
    elapsed_seconds = end_time - start_time;
    std::cout << "Total time XiTAO multiparallel region in iter " << iter << " : " << elapsed_seconds.count()  << std::endl;
    std::cout << "Total successful steals: " << tao_total_steals << std::endl;      
  }
 for (int iter = 0; iter < iter_count; ++iter) {  
    // invalidate the caches 
    for(int r = 0 ; r < N; ++r) {
      A[r] = new int[N];
      std::memset(A[r], rand(), sizeof(int) * N);
      B[r] = new int[N];
      std::memset(B[r], rand(), sizeof(int) * N);
      C[r] = new int[N];
      std::memset(C[r], rand(), sizeof(int) * N);
    }  
  start_time = std::chrono::system_clock::now();
#pragma omp parallel for
  for(int i = 0; i < N; ++i)
    for (int j = 0; j < N; ++j) 
     { 
       C[i][i] = 0; 
       for (int k = 0; k < N; ++k) 
         C[i][j] += A[i][k] *  
                    B[k][j]; 
     }  
  end_time = std::chrono::system_clock::now();
  elapsed_seconds = end_time - start_time;
  std::cout << "Total time OpenMP in iter " << iter << " : " << elapsed_seconds.count()  << std::endl;
  }
}
