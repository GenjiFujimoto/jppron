#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "util.h"

void* xmalloc(size_t nbytes)
{
    void* p = malloc(nbytes);
    if (!p)
	fatal_perror("malloc");
    return p;
}

void* xcalloc(size_t num, size_t nbytes)
{
    void* p = calloc(num, nbytes);
    if (!p)
	fatal_perror("calloc");
    return p;
}

void* xrealloc(void* ptr, size_t nbytes)
{
    void* p = realloc(ptr, nbytes);
    if (!p)
	fatal_perror("realloc");
    return p;
}

/* --------------- Start s8 utils -------------- */
void
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
	if (d)
	    return d;
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
	       .s = new(u8, len + 1),
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
    s8 s = { 0 };
    s.s = (u8 *)z;
    s.len = z ? strlen(z) : 0;
    return s;
}

i32
s8equals(s8 a, s8 b)
{
    return a.len == b.len && !u8compare(a.s, b.s, a.len);
}

s8
s8striputf8chr(s8 s)
{
    // 0x80 = 10000000; 0xC0 = 11000000
    assert(s.len > 0);
    s.len--;
    while (s.len > 0 && (s.s[s.len] & 0x80) != 0x00 && (s.s[s.len] & 0xC0) != 0xC0)
	s.len--;
    return s;
}

s8
s8unescape(s8 str)
{
    size s = 0, e = 0;
    while (e < str.len)
    {
	if (str.s[e] == '\\' && e + 1 < str.len)
	{
	    switch (str.s[e + 1])
	    {
	    case 'n':
		str.s[s++] = '\n';
		e += 2;
		break;
	    case '\\':
		str.s[s++] = '\\';
		e += 2;
		break;
	    case '"':
		str.s[s++] = '"';
		e += 2;
		break;
	    case 'r':
		str.s[s++] = '\r';
		e += 2;
		break;
	    default:
		str.s[s++] = str.s[e++];
	    }
	}
	else
	    str.s[s++] = str.s[e++];
    }
    str.len = s;

    return str;
}

s8
s8concat_(s8 strings[static 1])
{
    size len = 0;
    for (s8* s = strings; !s8equals(*s, s8("S8CONCAT_STOPPER")); s++)
	  len += s->len;

    s8 ret = news8(len);
    s8 p = ret;
    for (s8* s = strings; !s8equals(*s, s8("S8CONCAT_STOPPER")); s++)
	  p = s8copy(p, *s);

    return ret;
}

s8
buildpath_(s8 pathcomps[static 1])
{
#ifdef _WIN32
    s8 sep = s8("\\");
#else
    s8 sep = s8("/");
#endif
    size pathlen = 0;
    bool first = true;
    for (s8* pc = pathcomps; !s8equals(*pc, s8("BUILD_PATH_STOPPER")); pc++)
    {
	  if (!first) pathlen += sep.len;
	  pathlen += pc->len;

	  first = false;
    }

    s8 retpath = news8(pathlen);
    s8 p = retpath;

    first = true;
    for (s8* pc = pathcomps; !s8equals(*pc, s8("BUILD_PATH_STOPPER")); pc++)
    {
	  if (!first) p = s8copy(p, sep);
	  p = s8copy(p, *pc);

	  first = false;
    }

    return retpath;
}

void
frees8(s8 z[static 1])
{
    free(z->s);
    *z = (s8){0};
}

void
frees8buffer(s8* buf)
{
    while (buf_size(buf) > 0)
	free(buf_pop(buf).s);
    buf_free(buf);
}

/* -------------- Start dictentry / dictionary utils ---------------- */

dictentry
dictentry_dup(dictentry de)
{
    // TODO: Handle out of memory
    return (dictentry){
	       .dictname = strdup(de.dictname),
	       .kanji = strdup(de.kanji),
	       .reading = strdup(de.reading),
	       .definition = strdup(de.definition)
    };
}

void
dictentry_free(dictentry de)
{
    free(de.dictname);
    free(de.kanji);
    free(de.reading);
    free(de.definition);
}

void
dictentry_print(dictentry de)
{
    printf("dictname: %s\n"
	   "kanji: %s\n"
	   "reading: %s\n"
	   "definition: %s\n",
	   de.dictname,
	   de.kanji,
	   de.reading,
	   de.definition);
}

void
dictionary_copy_add(dictentry* dict[static 1], dictentry de)
{
    buf_push(*dict, dictentry_dup(de));
}

size
dictlen(dictentry* dict)
{
    return buf_size(dict);
}

void
dictionary_free(dictentry* dict[static 1])
{
    while (buf_size(*dict) > 0)
	dictentry_free(buf_pop(*dict));
    buf_free(*dict);
}

dictentry
dictentry_at_index(dictentry* dict, size index)
{
    assert(index >= 0 && (size_t)index < buf_size(dict));
    return dict[index];
}

/* -------------- End dictentry / dictionary utils ---------------- */

static char*
remove_substr(char str[static 1], char target[static 1])
{
    assert(str && target);

    size_t target_len = strlen(target);
    char *s = str, *e = str;

    do{
	while (strncmp(e, target, target_len) == 0)
	{
	    e += target_len;
	}
    } while ((*s++ = *e++));

    return str;
}

void
nuke_whitespace(char str[static 1])
{
    remove_substr(str, "\n");
    remove_substr(str, "\t");
    remove_substr(str, " ");
    remove_substr(str, "　");
}
