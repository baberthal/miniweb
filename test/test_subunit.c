#include <stdio.h>
#include <stdlib.h>
#include <subunit/child.h>
#include <unistd.h>

int main(void)
{
    subunit_progress(SUBUNIT_PROGRESS_SET, 10);
    subunit_test_start("test case");
    subunit_progress(SUBUNIT_PROGRESS_PUSH, 0);
    subunit_test_pass("test case");
    subunit_progress(SUBUNIT_PROGRESS_POP, 0);
    subunit_test_start("test case 2");
    subunit_progress(SUBUNIT_PROGRESS_CUR, 1);
    subunit_test_pass("test case 2");
    return 0;
}

/* vim: set ts=8 sw=4 tw=0 ft=c et :*/
