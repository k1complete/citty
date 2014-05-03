#include <sys/types.h>
#include <stdio.h>

size_t strlcpy(char * restrict dst, const char * restrict src, size_t size)
{
        int ret;
        ret = snprintf(dst, size, "%s", src);
        return ret;
}
