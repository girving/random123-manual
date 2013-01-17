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
/*
 * Known Answer Test program for Random123 Bijections. Reads lines that
 * specify a bijection, counter, key with known results and verifies that
 * the specified bijection on that counter,key produces the same
 * results.  The goal is to check for bugs when changing the code,
 * or trying new compilers, platforms etc.  This makes no attempt to
 * verify randomness, it only checks reproducibility of known results.
 * To verify randomness, use TestU01 SmallCrush/Crush/BigCrush.
 */
#include <Random123/philox.h>
#include <Random123/threefry.h>
#include <Random123/ars.h>
#include <Random123/aes.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include "util_macros.h"

#define MAXLINELEN 1024

const char *progname = NULL;
static int errs = 0;	/* accumulate error count for exit status */
static int verbose = 1;	/* 1 means print FAIL, 2 means print OK as well, 0 is quiet */
int debug = 0;

/* Verify that EXPR produces EXPECTEDVAL */
#define CHECKVAL(EXPR, EXPECTEDVAL) \
    do { \
	int val = (EXPR); \
	if (val != (EXPECTEDVAL)) { \
	    fprintf(stderr, "%s: Error: " #EXPR " returned %d, not %d\n", \
		    progname, val, EXPECTEDVAL); \
	    errs++; \
	} \
    } while(0)

/*
 * Template for a test function that reads in the data from a
 * line.  The main program will already have read the test name
 * and rounds from the input line, to determine which do_*
 * function to call, so this test function will now deal with the
 * rest of the line, which should consist of some number of hex
 * numbers for the counter, the key and the expected result.
 */
#define TEST_TPL(NAME, N, W) \
void do_##NAME##N##x##W(int rounds, char *line, int linenum) \
{ \
    NAME##N##x##W##_ctr_t ctr; \
    NAME##N##x##W##_ukey_t ukey; \
    NAME##N##x##W##_key_t key; \
    NAME##N##x##W##_ctr_t result; \
    NAME##N##x##W##_ctr_t R_unspecifiedresult; \
    NAME##N##x##W##_ctr_t expected; \
    int ret, i; \
    char ofmt[16]; \
    char *cp = line; \
    \
    /* \
     * use %lu and the cast (instead of %z) for portability to \
     * a widely-used compiler that refuses to implement C99,
     * sizeof(v[0]) should fit easily. \
     * Similarly, avoid PRIx##W \
     */ \
    sprintf(ofmt, " %%0%lullx", (unsigned long)sizeof(result.v[0])*2); \
    CHECKVAL(sizeof(ctr.v)/sizeof(ctr.v[0]), (N)); \
    CHECKVAL(sizeof(ctr.v[0]), (W)/8); \
    CHECKVAL(sizeof(expected.v)/sizeof(expected.v[0]), (N)); \
    CHECKVAL(sizeof(expected.v[0]), (W)/8); \
    CHECKVAL(sizeof(result.v)/sizeof(result.v[0]), (N)); \
    CHECKVAL(sizeof(result.v[0]), (W)/8); \
    CHECKVAL(sizeof(R_unspecifiedresult.v)/sizeof(R_unspecifiedresult.v[0]), (N)); \
    CHECKVAL(sizeof(R_unspecifiedresult.v[0]), (W)/8); \
    SCANFARRAY(ctr, NAME, N, W); \
    SCANFARRAY(ukey, NAME, sizeof(ukey.v)/sizeof(ukey.v[0]), W); \
    SCANFARRAY(expected, NAME, N, W); \
    key = NAME##N##x##W##keyinit(ukey); \
    result = NAME##N##x##W##_R(rounds, ctr, key); \
    /* if the round count matches the default-round-count, call the PRNG \
       without R.  Otherwise, set R_unspecified_result   \
       to the result obtained with R specified, making the \
       test of R_unspecifiedresult moot.  */ \
    if( rounds == NAME##N##x##W##_rounds ) { \
        R_unspecifiedresult = NAME##N##x##W(ctr, key); \
    } else {                                      \
        R_unspecifiedresult = result;                  \
    }                                             \
    for (i = 0; i < (N); i++) { \
	if (result.v[i] != expected.v[i] || R_unspecifiedresult.v[i] != expected.v[i]) { \
	    if (verbose > 0) { \
		fprintf(stderr, "%s: FAIL line %d: " #NAME #N "x" #W " %2d %s => ", progname, \
			linenum, rounds, line); \
		PRINTLINE(NAME, N, W, rounds, ctr, ukey, result, stderr); \
		fprintf(stderr, #NAME #N "x" #W " %2d expected was: ", rounds); \
		PRINTARRAY(expected, stderr); \
		putc('\n', stderr); \
	    } \
	    errs++; \
	    return; \
	} \
    } \
    if (verbose > 1) printf("OK " #NAME #N "x" #W " %2d %s", rounds, line); \
}

TEST_TPL(philox, 2, 32)
TEST_TPL(philox, 4, 32)
#if R123_USE_PHILOX_64BIT
TEST_TPL(philox, 2, 64)
TEST_TPL(philox, 4, 64)
#endif
TEST_TPL(threefry, 2, 64)
TEST_TPL(threefry, 4, 64)
TEST_TPL(threefry, 4, 32)

#if R123_USE_AES_NI
TEST_TPL(ars, 4, 32)
TEST_TPL(aesni, 4, 32)
#endif

#define ENTRY(NAME, N, W) { #NAME #N "x" #W, do_##NAME##N##x##W }

struct PrngEntry {
    char *name;
    void (*function)(int rounds, char *line, int linenum);
} prngs[] = {
    ENTRY(philox, 2, 32),
    ENTRY(philox, 4, 32),
#if R123_USE_PHILOX_64BIT
    ENTRY(philox, 2, 64),
    ENTRY(philox, 4, 64),
#endif
    ENTRY(threefry, 2, 64),
    ENTRY(threefry, 4, 64),
    ENTRY(threefry, 4, 32),
#if R123_USE_AES_NI
    ENTRY(ars, 4, 32),
    ENTRY(aesni, 4, 32),
#endif
};
    
/* strdup may or may not be in string.h, depending on the value
   of the pp-symbol _XOPEN_SOURCE and other arcana.  Just
   do it ourselves... */
char *ntcsdup(const char *s){
    char *p = malloc(strlen(s)+1);
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

int main(int argc, char **argv){
    char linebuf[MAXLINELEN], *line, prngname[MAXLINELEN];
    char filename[MAXLINELEN];
    int i, k, r, nt;
    FILE *inpfile;
    const char *inname;
    
    progname = argv[0];

    /* If there's an argument, open that file.
       else if getenv("srcdir") is non-empty open getenv("srcdir")/kat_vectors
       else open "./kat_vectors" */
    if( argc > 1 )
        inname = argv[1];
    else{
        char *e = getenv("srcdir");
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
    if ((line = getenv("KATC_VERBOSE"))) {
	verbose = atoi(line);
    }
    if ((line = getenv("KATC_DEBUG"))) {
	debug = atoi(line);
    }
#if R123_USE_AES_NI
    if (! haveAESNI()) {
	for (i = 0; i < sizeof(prngs)/sizeof(prngs[0]); i++) {
	    if (verbose > 1) printf("%d %s\n", i, prngs[i].name);
	    if (strncmp(prngs[i].name, "ars", 3) == 0 ||
		strncmp(prngs[i].name, "aes", 3) == 0) {
		prngs[i].name = "unavailable";
	    }
	}
    }
#endif
    k = 0;
    nt = 0;
    while ((line = fgets(linebuf, sizeof linebuf, inpfile)) != NULL) {
	k++;
	while (*line && isspace(*line)) {
	    line++;
	}
	if (!*line || *line == '#')
	    continue;
	if (debug)
	    printf("%d: read line: %s", k, line);
	fflush(stdout);
	if (sscanf(line, " %s %d %n", prngname, &r, &i) < 2) {
	    fprintf(stderr, "%s: input error on line %d: first word must be name rounds, skipping %s",
		    progname, k, linebuf);
	    errs++;
	    continue;
	}
	line += i;
	if (debug)
	    printf("prng \"%s\" %d line: %s\n", prngname, i, line);
	for (i = 0; i < sizeof(prngs)/sizeof(prngs[0]); i++) {
	    if (strcmp(prngname, prngs[i].name) == 0) {
		nt++;
		prngs[i].function(r, line, k);
		break;
	    }
	}
	if (i == sizeof(prngs)/sizeof(prngs[0])) {
            register_unknown(prngname);
	}
	fflush(stdout);
    }

    report_unknowns();

    if (ferror(inpfile)) {
	fprintf(stderr, "%s: input error %s\n", progname, strerror(errno));
	exit(1);
    }
    if(errs){
        printf("FAILED %d errors (%d tests run)\n", errs, nt);
        exit(2);
    }
    printf("OK all %d tests passed\n", nt);
    exit(0);
}

