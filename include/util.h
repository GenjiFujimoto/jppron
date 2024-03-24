#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <stddef.h> // size_t
#include <stdio.h> // perror

#include "buf.h"

typedef uint8_t    u8;
typedef int32_t    i32;
typedef signed int b32;
typedef uint32_t   u32;
typedef ptrdiff_t  size;

#ifdef DEBUG
#  if __GNUC__
#    define assert(c) if (!(c)) __builtin_trap()
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

// TODO: implement debug switch
#define debug_msg(...) 	           	  \
    do {			   	  \
	  if (1)			  \
	  {				  \
		fputs("DEBUG: ", stdout); \
		printf(__VA_ARGS__);      \
		putchar('\n');	   	  \
	  }				  \
    } while (0)

#define error_msg(...)                  \
    do {			        \
	  fprintf(stderr, __VA_ARGS__); \
	  fputc('\n', stderr);          \
    } while (0)

#define fatal(...)                \
    do {			  \
	  error_msg(__VA_ARGS__); \
	  exit(EXIT_FAILURE);     \
    } while (0)

#define fatal_perror(context)	  \
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
typedef struct {
    u8* s;
    size len;
} s8;

i32 u8compare(u8 *a, u8 *b, size n);
s8 s8copy(s8 dst, s8 src);
s8 news8(size len);
s8 s8dup(s8 s);
s8 fromcstr_(char* z);
i32 s8equals(s8 a, s8 b);
void frees8buffer(s8* buf);

#endif
