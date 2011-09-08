#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include "CuTest.h"

CuSuite* test_icd_list__get_suite(void);

void RunAllTests(void)
{
    CuString *output = CuStringNew();
    CuSuite* suite = CuSuiteNew();

    CuSuiteAddSuite(suite, test_icd_list__get_suite());

    CuSuiteRun(suite);
    CuSuiteSummary(suite, output);
    CuSuiteDetails(suite, output);
    printf("%s\n", output->buffer);
}

int main(void)
{
    struct rlimit l;
    memset(&l, 0, sizeof(l));
    l.rlim_cur = RLIM_INFINITY;
    l.rlim_max = RLIM_INFINITY;
    if (setrlimit(RLIMIT_CORE, &l)) {
    }
    RunAllTests();
    return 0;
}
