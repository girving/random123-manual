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
// Known Answer Test for C++ interface to Random123 Bijections.

#include <Random123/philox.h>
#include <Random123/threefry.h>
#include <Random123/ars.h>
#include <Random123/aes.h>
#include <Random123/conventional/Engine.hpp>
#include <Random123/MicroURNG.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <cstring>
#include <errno.h>

using namespace r123;

const char *progname;
int debug = 0;
unsigned int skipped = 0, discarded = 0;

using namespace std;

template<typename T>
int do_bijection(string& name, int linenum, istringstream& ss)
{
    if (debug) cout << "name = " << name << " rounds " << T::rounds << endl;
    T b;
    typename T::ctr_type ctr, result, expected;
    typename T::ukey_type ukey;
    typename T::key_type key;

    ss >> hex;
    ss >> ctr;
    ss >> ukey;
    key = ukey;
    ss >> expected;
    if (debug) {
	cout << "ctr: " << hex << ctr << endl;
	cout << "ukey: " << hex << ukey << endl;
	cout << "expected: " << hex << expected << endl;
    }
    result = b(ctr, key);
    if (debug) cout << "result: " << result << endl;
    if (result != expected) {
	cerr << name << " error: line " << dec << linenum << ": " << ss.str() << endl;
	//ARS and AES print the 16x8 badly here.
	cerr << "  got: " << hex << result << endl;
	return 1;
    }
    int errs = 0;

    // Test that an Engine produces same vectors.  The Engine implementation
    // returns words ctr.size(), .. 0, so one needs to initialize the engine
    // counter with elem = ctr.size() (i.e. N), and expect the words
    // back in that order.
    Engine<T> e;
    e.seed(key);
    typename T::ctr_type::value_type c0 = ctr[0];
    ctr[0] /= 3;
    e.setcounter(ctr, int(ctr.size()));
    // exercise discard if possible
    if (c0 > ctr[0]) {
	// get one random value
	(void) e();
	if (c0 > ctr[0]+1) {
	    // discard (skip over) many values
	    R123_ULONG_LONG ndiscard = (c0 - ctr[0] - 1);
	    ndiscard *= ctr.size();
	    e.discard(ndiscard);
	    discarded++;
	}
	// get ctr.size() - 1 values
	for (size_t i = 1; i < ctr.size(); i++) {
	    (void) e();
	}
    }
    for (size_t i = 0; i < expected.size(); i++) {
	typename T::ctr_type::value_type val = e();
	size_t j = expected.size() - i - 1;
	if (expected[j] != val) {
	    cerr << name << " error line " << dec << linenum << ": engine " << i << " returned "
		<< hex << val << ", expected " << j << " is " << expected[j] << endl;
	    errs++;
	}
    }
    ctr[0] = c0;

    // Now test ctr.size() words from a MicroURNG.  Since this MicroURNG needs the top 1 bit
    // of ctr[ctr.size()-1] to be zero, we skip any tests where that bit is set.
    typename T::ctr_type::value_type hibit = (ctr[ctr.size()-1] & ((typename T::ctr_type::value_type)1 << (sizeof(ctr.v[0])*8-1)));
    if (0 ==  hibit) {
        MicroURNG<T, 1> urng(ctr, key);
	for (size_t i = 0; i < expected.size(); i++) {
	    typename T::ctr_type::value_type val = urng();
	    size_t j = expected.size() - i - 1;
	    if (expected[j] != val) {
		cerr << name << " error line " << dec << linenum << ": microurng " << i << " returned "
		    << hex << val << ", expected " << j << " is " << expected[j] << endl;
		errs++;
	    }
	}
    } else {
	skipped++;
    }
	
    return errs;
}

int no_aes_ni(string& name, int linenum, istringstream& ss)
{
    cerr << progname << ": AES-NI not available for line " << linenum << ": ./" << name << endl;
    return 1;
}

#define ENTRY(NAME, N, W, R) { #NAME #N "x" #W, R, do_bijection<NAME##N##x##W##_R<R> > }
// Also test the no-R bijections, e.g., Threefry4x32
#define ENTRY_NO_R(NAME, N, W) { #NAME #N "x" #W, NAME##N##x##W::rounds, do_bijection<NAME##N##x##W> }

struct PrngEntry {
    string name;
    int rounds;
    int (* function)(string&, int, istringstream &);
} prngs[] = {
    ENTRY(Philox, 2, 32, 7),
    ENTRY(Philox, 2, 32, 10),
    ENTRY_NO_R(Philox, 2, 32),
    ENTRY(Philox, 4, 32, 7),
    ENTRY(Philox, 4, 32, 10),
    ENTRY_NO_R(Philox, 4, 32),
#if R123_USE_PHILOX_64BIT
    ENTRY(Philox, 2, 64, 7),
    ENTRY(Philox, 2, 64, 10),
    ENTRY_NO_R(Philox, 2, 64),
    ENTRY(Philox, 4, 64, 7),
    ENTRY(Philox, 4, 64, 10),
    ENTRY_NO_R(Philox, 4, 64),
#endif
    ENTRY(Threefry, 2, 64, 13),
    ENTRY(Threefry, 2, 64, 20),
    ENTRY_NO_R(Threefry, 2, 64),
    ENTRY(Threefry, 4, 32, 13),
    ENTRY(Threefry, 4, 32, 20),
    ENTRY_NO_R(Threefry, 4, 32),
    ENTRY(Threefry, 4, 64, 13),
    ENTRY(Threefry, 4, 64, 20),
    ENTRY_NO_R(Threefry, 4, 64),
#if R123_USE_AES_NI
    ENTRY(ARS, 4, 32, 7),
    ENTRY(ARS, 4, 32, 10),
    ENTRY_NO_R(ARS, 4, 32),
    ENTRY(AESNI, 4, 32, 10),
    ENTRY_NO_R(AESNI, 4, 32),
#endif
};

int
main(int argc, char **argv)
{
    istream *inp = NULL;
    progname = argv[0];

    map<string, int> unknowns;

    char *cp;
    if ((cp = getenv("KATPP_DEBUG"))) {
	debug = ::atoi(cp);
    }

    /* If there's an argument, open that file.
       else if getenv("srcdir") is non-empty open getenv("srcdir")/kat_vectors
       else open "./kat_vectors" */
    string inname;
    if( argc > 1 )
        inname = argv[1];
    else{
        const char *e = getenv("srcdir");
        if(!e)
            e = ".";
        inname = e + string("/kat_vectors");
    }

    if (inname == "-") {
	inp = &cin;
    } else {
	inp = new ifstream(inname.c_str(), ifstream::in);
	if (inp->fail()) {
	    cerr << progname << ": error opening input file " << argv[1]
		<< " for reading: " << ::strerror(errno) << endl;
	    exit(1);
	}
    }

    for (size_t i = 0; i < sizeof(prngs)/sizeof(prngs[0]); i++) {
	string *sp = &prngs[i].name;
	std::transform(sp->begin(), sp->end(), sp->begin(), ::tolower);
    }
#if R123_USE_AES_NI
    if (! haveAESNI()) {
	for (size_t i = 0; i < sizeof(prngs)/sizeof(prngs[0]); i++) {
	    string first3 = prngs[i].name.substr(0, 3);
	    if (debug) cout << i << " " << prngs[i].name << " " << first3 << endl;
	    if (first3 == "ars" || first3 == "aes") {
		prngs[i].name = "unavailable";
	    }
	}
    }
#endif

    int n, nt, errs;
    n = nt = errs = 0;
    *inp >> skipws;
    while (!inp->eof()) {
	string line, name;
	int rounds = 0;
	getline(*inp, line);
	n++;
	if (debug) cout << "Read line " << dec << n << " '" << line << "' " << endl;
	istringstream linestream(line);
	linestream >> name;
	if (name[0] == '#' || name == "") continue;
	linestream >> rounds;
        streampos sp = linestream.tellg();
	if (debug) cout <<  "Name " << " '" << name << "' rounds " << rounds << endl;
	size_t i;
        int nt0 = nt;
	for (i = 0; i < sizeof(prngs)/sizeof(prngs[0]); i++) {
	    if (name == prngs[i].name && rounds == prngs[i].rounds) {
                // There may be more than one match, so rewind the linestream
                // and clear its eof marker.
                linestream.seekg(sp); linestream.clear();
		errs += prngs[i].function(name, n, linestream);
		nt++;
	    }
	}
	if (nt == nt0) {
            if( unknowns.count(name) ){
                unknowns[name]++;
            }else{
                cerr << progname << ": unknown PRNG name or rounds, input line " << dec << n << ": " << name << " " << rounds << endl;
                unknowns[name] = 1;
            }
	}
    }
    if( !unknowns.empty() ){
        for(map<string, int>::iterator p=unknowns.begin(); p!=unknowns.end(); ++p){
            cout << p->second << " test vectors of type " << p->first << " skipped\n";
        }
    }
    if (skipped != 0) {
	cout << "Skipped " << dec << skipped << " MicroURNG tests because high-bit was set." << endl;
    }
    if (discarded != 0) {
	cout << "Discarded " << dec << discarded << " times from Engines." << endl;
    }
    if (errs == 0) {
	cout << "OK " << dec << nt << " tests passed" << endl;
    } else {
	cout << "FAILED " << dec << errs << " errors, (" << nt << " tests run)" << endl;
    }
    return errs > 0 ? 1 : 0;
}
