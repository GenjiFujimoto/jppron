#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdint.h>

#include "platformdep.h"
#include "buf.h" // Growable buffer implementation

typedef uint8_t    u8;
typedef int32_t    i32;
typedef signed int b32;
typedef uint32_t   u32;
typedef ptrdiff_t  size;

#ifdef DEBUG
#  if __GNUC__
#    define assert(c)  while (!(c)) __builtin_unreachable()
#  elif _MSC_VER
#    define assert(c) if (!(c)) __debugbreak()
#  else
#    define assert(c) if (!(c)) *(volatile int *)0 = 0
#  endif
#else
#  define assert(c)
#endif

#define msg(...) 	          \
    do {			  \
	  printf(__VA_ARGS__);    \
	  putchar('\n');	  \
    } while (0)

#define debug_msg(...) 	           	  \
    do {			   	  \
	  if (1)			  \
	  {				  \
		fputs("DEBUG: ", stdout); \
		printf(__VA_ARGS__);      \
		putchar('\n');	   	  \
	  }				  \
    } while (0)

#define error_msg(...)                                     \
    do {			                           \
	  fprintf(stderr, "%s (%d):", __FILE__, __LINE__); \
	  fprintf(stderr, __VA_ARGS__); 		   \
	  fputc('\n', stderr);                             \
    } while (0)

#define fatal(...)                \
    do {			  \
	  error_msg(__VA_ARGS__); \
	  exit(EXIT_FAILURE);     \
    } while (0)

#define fatal_perror(context)     \
    do {			  \
	  perror(context);        \
	  exit(EXIT_FAILURE);     \
    } while (0)

/*
 * A malloc wrapper, which aborts on insufficient memory
 */ 
void* xmalloc(size_t size);
void* xcalloc(size_t nmemb, size_t size);
void* xrealloc(void* ptr, size_t size);
#define new(type, num) xcalloc(num, sizeof(type))

#define countof(a)    (size)(sizeof(a) / sizeof(*(a)))
#define lengthof(s)   (countof("" s "") - 1)
#define s8(s)         (s8){(u8 *)s, countof(s)-1}
typedef struct s8 {
    u8* s;
    size len;
} s8;

void u8copy(u8 *dst, u8 *src, size n);
i32 u8compare(u8 *a, u8 *b, size n);
/*
 * Copies @src into @dst returning the remaining portion of @dst
 */
s8 s8copy(s8 dst, s8 src);

s8 news8(size len);
/*
 * Returns a copy of s
 */
s8 s8dup(s8 s);
/*
 * Turns @z into an s8 string, reusing the pointer.
 */
s8 fromcstr_(char* z);
/*
 * Returns a true value if a is equal to b
 */
i32 s8equals(s8 a, s8 b);
/*
 * Returns s with the last UTF-8 character removed
 * Expects s to have length > 0
 */
s8 s8striputf8chr(s8 s);
/*
 * Turns escaped characters such as the string "\\n" into the character '\n' (inplace)
 */
s8 s8unescape(s8 str);

void frees8(s8 z[static 1]);
void frees8buffer(s8* buf);

#define s8concat(...) \
    s8concat_((s8[]){ __VA_ARGS__, s8("S8CONCAT_STOPPER") });
s8 s8concat_(s8 strings[static 1]);

#define buildpath(...) \
    buildpath_((s8[]){ __VA_ARGS__, s8("BUILD_PATH_STOPPER") });
s8 buildpath_(s8 pathcomps[static 1]);


/* --------------------- Start dictentry / dictionary ----------------- */
typedef struct {
  char *dictname;
  char *kanji;
  char *reading;
  char *definition;
} dictentry;

dictentry dictentry_dup(dictentry de);
void dictentry_print(dictentry de);
void dictionary_copy_add(dictentry* dict[static 1], dictentry de);
size dictlen(dictentry* dict);
void dictionary_free(dictentry* dict[static 1]);
dictentry dictentry_at_index(dictentry* dict, size index);
/* --------------------- End dictentry ------------------------ */

void nuke_whitespace(char str[static 1]);

int printf_cmd_async(char const *fmt, ...);

#endif
