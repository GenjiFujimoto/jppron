#include <stddef.h>
#include <stdlib.h>

#include "util.h"

void*
xcalloc(size_t num, size_t nbytes)
{
    void* p = calloc(num, nbytes);
    if (!p) fatal_perror("calloc");
    return p;
}

/* --------------- Start s8 utils -------------- */
static void
u8copy(u8* dst, u8* src, size n)
{
	assert(n >= 0);
	for (; n; n--)
	{
		*dst++ = *src++;
	}
}

i32
u8compare(u8* a, u8* b, size n)
{
	for (; n; n--)
	{
		i32 d = *a++ - *b++;
		if (d) return d;
	}
	return 0;
}

/* 
 * Copy src into dst returning the remaining portion of dst.
 */
s8
s8copy(s8 dst, s8 src)
{
	assert(dst.len >= src.len);

	u8copy(dst.s, src.s, src.len);
	dst.s += src.len;
	dst.len -= src.len;
	return dst;
}

s8
news8(size len)
{
    return (s8) {
	       .s = new(u8, len + 1), // Include 0 terminator
	       .len = len
    };
}

s8
s8dup(s8 s)
{
	s8 r = news8(s.len);
	u8copy(r.s, s.s, s.len);
	return r;
}

s8
fromcstr_(char* z)
{
    s8 s = {0};
    s.s = (u8*)z;
    if (s.s) {
        for (; s.s[s.len]; s.len++) {}
    }
    return s;
}

i32
s8equals(s8 a, s8 b)
{
	return a.len == b.len && !u8compare(a.s, b.s, a.len);
}

void
frees8buffer(s8* buf)
{
    while (buf_size(buf) > 0)
	  free(buf_pop(buf).s);
    buf_free(buf);
}
/* ----------- End s8 utils --------------------------- */
