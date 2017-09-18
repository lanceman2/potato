#include <stdbool.h>
#include "debug.h"

int main(void)
{
    int i=0;
    i++;
    poDebugInit();
    SPEW_LEVEL(PO_SPEW_INFO);
    DSPEW();
    DSPEW("MORE");
    INFO();
    INFO("MORE data=%d", 435);
    NOTICE();
    NOTICE("notice with data=\"%s\"", "DATA_zxc324");
    WARN();
    WARN("warning with data=%p", &i);
    ERROR();
    ERROR("error with just text %s", "rrr");
    SPEW();
    SPEW("SPEW with just text %s", "rrr");
    DASSERT(1);
    DASSERT(0);
    ASSERT(3);
    VASSERT(0, "with text=%s", "more text");
    FAIL();
    FAIL("FAIL with data=\"%s\"", "DATA_zxc324");

    return 0;
}
