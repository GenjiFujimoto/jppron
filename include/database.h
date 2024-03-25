#include <stdbool.h>
#include "util.h"

typedef struct {
  void* value;
  size len;
} data_s;

void opendb(const char* path, bool write);
void closedb(void);
/*
 * Add to database, allowing duplicates if they are added directly after another
 */
void addtodb1(s8 key, s8 val);
void addtodb2(s8 key, s8 val);

s8* getfiles(s8 key);
s8 getfromdb2(s8 key);
