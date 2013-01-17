/* Known Answer Test */

/* We use the same source files to implement the Known Answer Test
   (KAT) in C, C++, OpenCL and CUDA.  Supporting all four environments
   with a single source file, and getting it all to work with 'make'
   is a bit involved:

   There are four "top-level" files:
     kat_c.c
     kat_cpp.cpp
     kat_cuda.c
     kat_opencl.c

   These correspond to make targets: kat_c, kat_cpp, kat_cuda
   and kat_opencl.

   Those four files are very simple.  First, they #include this file,
   which contains all the machinery for reading test vectors,
   complaining about errors, etc..  Then they implement the function
   host_execute_tests() in the appropriate environment.  host_execute_tests
   looks very different in C/C++/CUDA/OpenCL.

   host_execute_tests contrives to call/launch "dev_execute_tests"
   on the device.  Except for a few environment-specific keywords,
   (e.g., __global, __kernel), which are #defined in kat_XXX.c,
   dev_execute_tests is obtained by including a common source file:
      #include <kat_dev_execute.c>

   One final complication:  in order to fully "bake" the source code
   into the binary at compile-time, dev_execute_tests for opencl is implemented in
   kat_opencl_kernel.ocl, which is processed by gencl.sh into
   kat_opencl_kernel.i, which is thein #include-ed by kat_opencl.c.
   
*/
#include "kat.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#define LINESIZE 1024

/* MSVC doesn't know about strtoull.  Strictly speaking, strtoull
   isn't standardized in C++98, either, but that seems not to be a
   problem so we blissfully ignore it and use strtoull (or its MSVC
   equivalent, _strtoui64) in both C and C++.  If strtoull in C++
   becomes a problem, we can adopt the prtu strategy (see below) and
   write C++ versions of strtouNN, that use an istringstream
   instead. */
#ifdef _MSC_FULL_VER
#define strtoull _strtoui64
#endif
uint32_t strtou32(const char *p, char **endp, int base){
    uint32_t ret;
    errno = 0;
    ret = strtoul(p, endp, base);
    assert(errno==0);
    return ret;
}
uint64_t strtou64(const char *p, char **endp, int base){
    uint64_t ret;
    errno = 0;
    ret = strtoull(p, endp, base);
    assert(errno==0);
    return ret;
}

#if defined(__cplusplus)
/* Strict C++98 doesn't grok %llx or unsigned long long, and with
   aggressive error-checking, e.g., g++ -pedantic -Wall, will refuse
   to compile code like:

     fprintf(stderr, "%llx", (R123_ULONG_LONG)v);

   On the other hand, when compiling to a 32-bit target, the only
   64-bit type is long long, so we're out of luck if we can't use llx.
   A portable, almost-standard way to do I/O on uint64_t values in C++
   is to use bona fide C++ I/O streams.  We are still playing
   fast-and-loose with standards because C++98 doesn't have <stdint.h>
   and hence doesn't even guarantee that there's a uint64_t, much less
   that the insertion operator<<(ostream&) works correctly with
   whatever we've typedef'ed to uint64_t in
   <features/compilerfeatures.h>.  Hope for the best... */
#include <iostream>
#include <limits>
template <typename T>
void prtu(T val){
    using namespace std;
    cerr.width(std::numeric_limits<T>::digits/4);
    char prevfill = cerr.fill('0');
    ios_base::fmtflags prevflags = cerr.setf(ios_base::hex, ios_base::basefield);
    cerr << val;
    cerr.flags(prevflags);
    cerr.fill(prevfill);
    assert(!cerr.bad());
}
void prtu32(uint32_t v){ prtu(v); }
void prtu64(uint64_t v){ prtu(v); }

#else /* __cplusplus */
/* C should be easy.  inttypes.h was standardized in 1999.  But Microsoft
   refuses to recognize the 12-year old standard, so: */
#if defined(_MSC_FULL_VER)
#define PRIx32 "x"
#define PRIx64 "I64x"
#else /* _MSC_FULL_VER */
#include <inttypes.h>
#endif /* _MSVC_FULL_VER */
void prtu32(uint32_t v){ fprintf(stderr, "%08" PRIx32, v); }
void prtu64(uint64_t v){ fprintf(stderr, "%016" PRIx64, v); }

#endif /* __cplusplus */

int have_aesni = 0;
int verbose = 0;
int debug = 0;
size_t nfailed = 0;
const char *progname;

extern void host_execute_tests(kat_instance *tests, size_t ntests);
                
/* strdup may or may not be in string.h, depending on the value
   of the pp-symbol _XOPEN_SOURCE and other arcana.  Just
   do it ourselves... */
char *ntcsdup(const char *s){
    char *p = (char *)malloc(strlen(s)+1);
    strcpy(p, s);
    return p;
}

/* A little hack to keep track of the test vectors that we don't know how to deal with: */
int nunknowns = 0;
#define MAXUNKNOWNS 20
const char *unknown_names[MAXUNKNOWNS];
int unknown_counts[MAXUNKNOWNS];

void register_unknown(const char *name){
    int i;
    for(i=0; i<nunknowns; ++i){
        if( strcmp(name, unknown_names[i]) == 0 ){
            unknown_counts[i]++;
            return;
        }
    }
    if( i >= MAXUNKNOWNS ){
        fprintf(stderr, "Too many unknown rng types.  Bye.\n");
        exit(1);
    }
    nunknowns++;
    unknown_names[i] = ntcsdup(name);
    unknown_counts[i] = 1;
}

void report_unknowns(){
    int i;
    for(i=0; i<nunknowns; ++i){
        printf("%d test vectors of type %s skipped\n", unknown_counts[i], unknown_names[i]);
    }
}

/* read_<GEN>NxW */
#define RNGNxW_TPL(base, N, W) \
int read_##base##N##x##W(const char *line, kat_instance* tinst){        \
    size_t i;                                                           \
    int nchar;                                                          \
    const char *p = line;                                               \
    char *newp;                                                         \
    size_t nkey = sizeof(tinst->u.base##N##x##W##_data.ukey.v)/sizeof(tinst->u.base##N##x##W##_data.ukey.v[0]); \
    tinst->method = base##N##x##W##_e;                                  \
    sscanf(p,  "%u%n", &tinst->nrounds, &nchar);                        \
    p += nchar;                                                         \
    for(i=0;  i<N; ++i){                                                \
        tinst->u.base##N##x##W##_data.ctr.v[i] = strtou##W(p, &newp, 16); \
        p = newp;                                                       \
    }                                                                   \
    for(i=0; i<nkey; ++i){                                              \
        tinst->u.base##N##x##W##_data.ukey.v[i] = strtou##W(p, &newp, 16); \
        p = newp;                                                       \
    }                                                                   \
    for(i=0;  i<N; ++i){                                                \
        tinst->u.base##N##x##W##_data.expected.v[i] = strtou##W(p, &newp, 16); \
        p = newp;                                                       \
    }                                                                   \
    memset(tinst->u.base##N##x##W##_data.computed.v, 0, sizeof(tinst->u.base##N##x##W##_data.computed.v));                  \
    return 1;                                                           \
}
#include "rngNxW.h"
#undef RNGNxW_TPL

/* readtest:  dispatch to one of the read_<GEN>NxW functions */
static int readtest(const char *line, kat_instance* tinst){
    int nchar;
    char name[LINESIZE];
    if( line[0] == '#') return 0;                                       
    sscanf(line, "%s%n", name, &nchar);
    if(!have_aesni){
        /* skip any tests that require AESNI */ 
        if(strncmp(name, "aes", 3)==0 ||
           strncmp(name, "ars", 3)==0){
            register_unknown(name);
            return 0;
        }
    }
#define RNGNxW_TPL(base, N, W) if(strcmp(name, #base #N "x" #W) == 0) return read_##base##N##x##W(line+nchar, tinst);
#include "rngNxW.h"
#undef RNGNxW_TPL

    register_unknown(name);
    return 0;
}

#define RNGNxW_TPL(base, N, W) \
void report_##base##N##x##W##error(const kat_instance *ti){ \
 size_t i;                                                     \
 size_t nkey = sizeof(ti->u.base##N##x##W##_data.ukey.v)/sizeof(ti->u.base##N##x##W##_data.ukey.v[0]); \
 fprintf(stderr, "FAIL:  expected: ");                                \
 fprintf(stderr, #base #N "x" #W " %d", ti->nrounds);                   \
 for(i=0; i<N; ++i){                                                    \
     fprintf(stderr, " "); prtu##W(ti->u.base##N##x##W##_data.ctr.v[i]); \
 }                                                                      \
 for(i=0; i<nkey; ++i){                                                 \
     fprintf(stderr, " "); prtu##W(ti->u.base##N##x##W##_data.ukey.v[i]); \
 }                                                                      \
 for(i=0; i<N; ++i){                                                    \
     fprintf(stderr, " "); prtu##W(ti->u.base##N##x##W##_data.expected.v[i]); \
 }                                                                      \
 fprintf(stderr, "\n");                                                 \
                                                                        \
 fprintf(stderr, "FAIL:  computed: ");                                \
 fprintf(stderr, #base #N "x" #W " %d", ti->nrounds);                   \
 for(i=0; i<N; ++i){                                                    \
     fprintf(stderr, " "); prtu##W(ti->u.base##N##x##W##_data.ctr.v[i]); \
 }                                                                      \
 for(i=0; i<nkey; ++i){                                                 \
     fprintf(stderr, " "); prtu##W(ti->u.base##N##x##W##_data.ukey.v[i]); \
 }                                                                      \
 for(i=0; i<N; ++i){                                                    \
     fprintf(stderr, " "); prtu##W(ti->u.base##N##x##W##_data.computed.v[i]); \
 }                                                                      \
 fprintf(stderr, "\n");                                                 \
 nfailed++;                                                             \
}
#include "rngNxW.h"
#undef RNGNxW_TPL

// dispatch to one of the report_<GEN>NxW() functions
void analyze_tests(const kat_instance *tests, size_t ntests){
    size_t i;
    for(i=0; i<ntests; ++i){
        const kat_instance *ti = &tests[i];
        switch(tests[i].method){
#define RNGNxW_TPL(base, N, W) case base##N##x##W##_e: \
            if (memcmp(ti->u.base##N##x##W##_data.computed.v, ti->u.base##N##x##W##_data.expected.v, N*W/8)) report_##base##N##x##W##error(ti); break;
#include "rngNxW.h"
#undef RNGNxW_TPL
        case unused: ; // silence a warning
        }
    }
}

#define NTESTS 1000

int main(int argc, char **argv){
    kat_instance *tests;
    size_t t, ntests = NTESTS;
    char linebuf[LINESIZE];
    FILE *inpfile;
    const char *p;
    const char *inname;
    char filename[LINESIZE];
    
    progname = argv[0];

    /* If there's an argument, open that file.
       else if getenv("srcdir") is non-empty open getenv("srcdir")/kat_vectors
       else open "./kat_vectors" */
    if( argc > 1 )
        inname = argv[1];
    else{
        const char *e = getenv("srcdir");
        if(!e)
            e = ".";
        sprintf(filename, "%s/kat_vectors", e);
        inname = filename;
    }

    if (strcmp(inname, "-") == 0) {
	inpfile = stdin;
    } else {
	inpfile = fopen(inname, "r");
	if (inpfile == NULL) {
	    fprintf(stderr, "%s: error opening input file %s for reading: %s\n",
		    progname, inname, strerror(errno));
	    exit(1);
	}
    }
    if ((p = getenv("KATC_VERBOSE")) != NULL) {
	verbose = atoi(p);
    }
    if ((p = getenv("KATC_DEBUG")) != NULL) {
	debug = atoi(p);
    }

#if R123_USE_AES_NI
    have_aesni = haveAESNI();
#else
    have_aesni = 0;
#endif

    tests = (kat_instance *) malloc(sizeof(tests[0])*ntests);
    if (tests == NULL) {
	fprintf(stderr, "Could not allocate %lu bytes for tests\n",
		(unsigned long) ntests);
	exit(1);
    }
    t = 0;
    while (fgets(linebuf, sizeof linebuf, inpfile) != NULL) {
        if( t==ntests ){
	    ntests *= 2;
	    tests = (kat_instance *)realloc(tests, sizeof(tests[0])*ntests);
	    if (tests == NULL) {
		fprintf(stderr, "Could not grow tests to %lu bytes\n",
			(unsigned long) ntests);
		exit(1);
	    }
        }
        if( readtest(linebuf, &tests[t]) )
            ++t;
    }

    report_unknowns();
    printf("Perform %lu tests.\n", (unsigned long)t);
    host_execute_tests(tests, t);

    analyze_tests(tests, t);
    free(tests);
    if(nfailed != 0){
        printf("FAILED %lu out of %lu\n", (unsigned long)nfailed, (unsigned long)t);
        return 1;
    }else{
        printf("PASSED %lu known answer tests\n", (unsigned long)t);
        return 0;
    }
}
