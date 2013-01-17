#include "kat.h"
KAT_KERNEL void dev_execute_tests(KAT_GLOBAL kat_instance *tests, KAT_UINT ntests){
    size_t i;
    for(i=0; i<ntests; ++i){
        KAT_GLOBAL kat_instance *ti = &tests[i];
        switch(tests[i].method){
            //case philox2x32_e: ti->philox2x32_data.computed = philox2x32_R(ti->rounds, ti->philox2x32_data.ctr, ti->philox2x32_data.key);
#define RNGNxW_TPL(base, N, W) case base##N##x##W##_e: ti->u.base##N##x##W##_data.computed = base##N##x##W##_R(ti->nrounds, ti->u.base##N##x##W##_data.ctr, base##N##x##W##keyinit(ti->u.base##N##x##W##_data.ukey)); break;
#include "rngNxW.h"
#undef RNGNxW_TPL
        case unused: ; // silence a warning
        }
    }
}
