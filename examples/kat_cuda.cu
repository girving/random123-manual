#include "util_cuda.h"
#include "kat.c"

#define KAT_KERNEL __global__
#define KAT_GLOBAL
#define KAT_UINT size_t
#include "kat_dev_execute.c"

void host_execute_tests(kat_instance *tests_host, size_t ntests){
    CUDAInfo *infop;
    kat_instance *tests_dev;
    size_t tests_sz;

    infop = cuda_init(NULL);

    tests_sz = ntests * sizeof(tests_host[0]);
    CHECKCALL(cudaMalloc(&tests_dev, tests_sz));
    CHECKCALL(cudaMemcpy(tests_dev, tests_host, tests_sz, cudaMemcpyHostToDevice));

    printf("starting %zu tests on 1 blocks with 1 threads/block\n", ntests);
    fflush(stdout);

    // TO DO:  call this with parallelism, <<<infop->blocks_per_grid, infop->threads_per_block>>>
    // and then insure that each of the threads got the same result.
    dev_execute_tests<<<1, 1>>>(tests_dev, ntests);

    CHECKCALL(cudaThreadSynchronize());
    CHECKCALL(cudaMemcpy(tests_host, tests_dev, tests_sz, cudaMemcpyDeviceToHost));
    cuda_done(infop);
}

