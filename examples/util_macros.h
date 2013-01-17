/*
Copyright 2010-2011, D. E. Shaw Research.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

* Redistributions of source code must retain the above copyright
  notice, this list of conditions, and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions, and the following disclaimer in the
  documentation and/or other materials provided with the distribution.

* Neither the name of D. E. Shaw Research nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef UT_MACROS_H__
#define UT_MACROS_H__ 1

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

extern int debug;
extern const char *progname;

#define CHECKNOTEQUAL(x, y)  if ((x) != (y)) ; else { \
    fprintf(stderr, "%s: %s line %d error %s == %s (%s)\n", progname, __FILE__, __LINE__, #x, #y, strerror(errno)); \
    exit(1); \
}
#define CHECKEQUAL(x, y)  if ((x) == (y)) ; else { \
    fprintf(stderr, "%s: %s line %d error %s != %s (%s)\n", progname, __FILE__, __LINE__, #x, #y, strerror(errno)); \
    exit(1); \
}
#define CHECKZERO(x)  CHECKEQUAL((x), 0)
#define CHECKNOTZERO(x)  CHECKNOTEQUAL((x), 0)

#define dprintf(x) if (debug < 1) ; else { printf x; fflush(stdout); }

#define ALLZEROS(x, K, N) \
do { \
    int allzeros = 1; \
    size_t xi, xj; \
    for (xi = 0; xi < (K); xi++) \
	for (xj = 0; xj < (N); xj++) \
	    allzeros = allzeros & ((x)[xi].v[xj] == 0); \
    if (allzeros) fprintf(stderr, "%s: Unexpected, all %lu elements of %ux%u had all zeros!\n", progname, (unsigned long)K, (unsigned)N, (unsigned)sizeof(x[0].v[0])); \
} while(0)

/* Read in N words of width W into ARR */
#define SCANFARRAY(ARR, NAME, N, W) \
do { \
    int xi, xj; \
    unsigned long long xv; \
    for (xi = 0; xi < (N); xi++) { \
        /* Avoid any cleverness with SCNx##W because Microsoft (as of Visual Studio 10.x) silently trashes the stack by pretending that %hhx is %x). */ \
	const char *xfmt = " %llx%n"; \
	ret = sscanf(cp, xfmt, &xv, &xj); \
	ARR.v[xi] = (uint##W##_t)xv; \
	if (debug > 1) printf("line %d: xfmt for W=%d is \"%s\", got ret=%d xj=%d, %s[%d]=%llx cp=%s", linenum, W, xfmt, ret, xj, #ARR, xi, (unsigned long long) ARR.v[xi], cp); \
	if (ret < 1) { \
	    fprintf(stderr, "%s: ran out of words reading %s on line %d: " #NAME #N "x" #W " %2d %s", \
		    progname, #ARR, linenum, rounds, line); \
	    errs++; \
	    return; \
	} \
	cp += xj; \
    } \
} while(0)

#define PRINTARRAY(ARR, fp) \
do { \
    char ofmt[64]; \
    size_t xj; \
    /* use %lu and the cast (instead of z) for portability to Microsoft, sizeof(v[0]) should fit easily in an unsigned long.  Avoid inttypes for the same reason. */ \
    sprintf(ofmt, " %%0%lullx", (unsigned long)sizeof(ARR.v[0])*2UL); \
    for (xj = 0; xj < sizeof(ARR.v)/sizeof(ARR.v[0]); xj++) { \
	fprintf(fp, ofmt, (unsigned long long) ARR.v[xj]); \
    } \
} while(0)

#define PRINTLINE(NAME, N, W, R, ictr, ukey, octr, fp) \
do { \
    fprintf(fp, "%s %d ", #NAME #N "x" #W, R); \
    PRINTARRAY(ictr, fp); \
    putc(' ', fp); \
    PRINTARRAY(ukey, fp); \
    putc(' ', fp); \
    PRINTARRAY(octr, fp); \
    putc('\n', fp); \
    fflush(fp); \
} while(0)

#endif /* UT_MACROS_H__ */
