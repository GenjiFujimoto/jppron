#include <stdbool.h>
#include "util.h"

void opendb(const char* path, bool write);
void closedb(void);
void addtodb(size_t db_nr, s8 key, s8 val);
s8* getfiles(s8 headword);
s8 getreading(s8 fn);
