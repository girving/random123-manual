#include "kat.c"

#define KAT_KERNEL
#define KAT_GLOBAL
#define KAT_UINT size_t
#include "kat_dev_execute.c"

void host_execute_tests(kat_instance *tests, size_t ntests){
    dev_execute_tests(tests, ntests);
}
