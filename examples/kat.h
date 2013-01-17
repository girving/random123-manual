#ifndef __katdoth__
#define __katdoth__

#include <Random123/philox.h>
#include <Random123/threefry.h>
#include <Random123/ars.h>
#include <Random123/aes.h>

enum method_e{
#define RNGNxW_TPL(base, N, W) base##N##x##W##_e,
#include "rngNxW.h"
#undef RNGNxW_TPL
    unused // silences warning about dangling comma
};

#define RNGNxW_TPL(base, N, W)                       \
    typedef struct {                                 \
        base##N##x##W##_ctr_t ctr;                   \
        base##N##x##W##_ukey_t ukey;                 \
        base##N##x##W##_ctr_t expected;              \
        base##N##x##W##_ctr_t computed;              \
    } base##N##x##W##_kat;
#include "rngNxW.h"
#undef RNGNxW_TPL

typedef struct{
    enum method_e method;
    unsigned nrounds;
    union{
#define RNGNxW_TPL(base, N, W) base##N##x##W##_kat base##N##x##W##_data;
#include "rngNxW.h"
#undef RNGNxW_TPL
    }u;
} kat_instance;

#endif
