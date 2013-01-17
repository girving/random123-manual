#include "util_opencl.h"
#include "kat.c"

// USE_GENCL:  gencl.sh is a small shell script
// that pre-processes foo.ocl into foo.i, containing
// a definition like:
//  const char opencl_src[] = "preprocessed text of foo.ocl"
// Thus, with gencl, this file says 
//    #include <foo.i>
// and the binary obtained by compiling it
// is fully "baked".  Runtime behavior doesn't depend
// on the contents of some file (e.g., foo.ocl or some
// header that it includes) that might have changed long after this
// file was compiled.
//
// The alternative (USE_GENCL 0) seems to be more along
// the lines of what OpenCL designers imagine.  It makes the text of the
// kernel program the string "#include <foo.c>".  This eliminates
// the need for the extra machinery in gencl.sh, but runtime
// behavior is susceptable to changes in foo.c, or files included
// by foo.c long after this file is compiled.  It also requires some
// hocus pocus to get absolute paths for the -I options needed
// to compile the code at runtime.  Something like:
//  override CFLAGS += -DSRCDIR=\"$(dir $(abspath $<)).\"
#define USE_GENCL 1

#if USE_GENCL
#include <kat_opencl_kernel.i>
#else
#ifndef SRCDIR
#error -DSRCDIR="/absolute/path/to/examples" should have been put on the command-line by GNUmakefile
#endif
#endif

void host_execute_tests(kat_instance *tests, size_t ntests){
    UCLInfo *infop;
    cl_kernel kern;
    size_t nthreads, tests_sz;
    cl_mem tests_dev;
    const char *kernelname = "dev_execute_tests";
    cl_int err;
    cl_uint cl_ntests = ntests;

#if USE_GENCL
    infop = opencl_init(NULL, opencl_src, "");
#else
    infop = opencl_init(NULL, "#include <kat_dev_execute.c>", 
                        " -I" SRCDIR 
                        " -I" SRCDIR "/../include " 
                        " -DKAT_KERNEL=__kernel "
                        " -DKAT_GLOBAL=__global "
                        " -DKAT_UINT=uint" );
#endif
    CHECKERR(kern = clCreateKernel(infop->prog, kernelname, &err));
    if (infop->wgsize > 64) infop->wgsize /= 2;
    nthreads = infop->cores * infop->wgsize;
    tests_sz = ntests * sizeof(*tests);
    CHECKERR(tests_dev = clCreateBuffer(infop->ctx, CL_MEM_READ_WRITE|CL_MEM_USE_HOST_PTR, tests_sz, tests, &err));
    CHECK(clEnqueueWriteBuffer(infop->cmdq, tests_dev, CL_TRUE, 0, tests_sz, tests, 0, 0, 0));
    CHECK(clSetKernelArg(kern, 0, sizeof(cl_mem), (void*)&tests_dev));
    CHECK(clSetKernelArg(kern, 1, sizeof(cl_uint), (void*)&cl_ntests));
    printf("queuing kernel for %lu threads with %lu work group size, %u tests\n",
	   (unsigned long)nthreads, (unsigned long)infop->wgsize, cl_ntests);
    CHECK(clEnqueueNDRangeKernel(infop->cmdq, kern, 1, 0, &nthreads, &infop->wgsize, 0, 0, 0));
    CHECK(clFinish(infop->cmdq));
    CHECK(clEnqueueReadBuffer(infop->cmdq, tests_dev, CL_TRUE, 0, tests_sz, tests, 0, 0, 0));
    CHECK(clReleaseMemObject(tests_dev));
    CHECK(clReleaseKernel(kern));
    opencl_done(infop);
}
