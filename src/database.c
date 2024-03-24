#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

#include "lmdb.h"
#include "util.h"

int rc;
#define MDB_CHECK(call)                                  \
	do {                                             \
	    if ((rc = call) != MDB_SUCCESS) {            \
		printf("Error: %s\n at %s (%d)\n", mdb_strerror(rc), __FILE__, __LINE__); \
		exit(EXIT_FAILURE);                      \
	    }                                            \
	} while (0)

MDB_env *env = 0;
MDB_dbi dbi1 = 0;
MDB_dbi dbi2 = 0;
MDB_txn *txn = 0;
bool READONLY = true;

void
opendb(const char* path, bool readonly)
{
    MDB_CHECK(mdb_env_create(&env));
    MDB_CHECK(mdb_env_set_maxdbs(env, 2));

    if (readonly)
    {
	READONLY = true;
	MDB_CHECK(mdb_env_open(env, path, MDB_RDONLY | MDB_NOLOCK | MDB_NORDAHEAD, 0664));
    }
    else
    {
	READONLY = false;
	unsigned int mapsize = 2147483648; // 1 Gb max
	MDB_CHECK(mdb_env_set_mapsize(env, mapsize));

	/* MDB_CHECK(mdb_env_open(env, path, MDB_WRITEMAP, 0664)); */
	MDB_CHECK(mdb_env_open(env, path, 0, 0664));
	MDB_CHECK(mdb_txn_begin(env, NULL, 0, &txn));
	MDB_CHECK(mdb_dbi_open(txn, "head-file", MDB_DUPSORT | MDB_CREATE, &dbi1));
	MDB_CHECK(mdb_dbi_open(txn, "file-info", MDB_DUPSORT | MDB_CREATE, &dbi2));
    }
}

void
closedb()
{
    if (!READONLY)
    {
	MDB_CHECK(mdb_txn_commit(txn));
	mdb_dbi_close(env, dbi1);
	mdb_dbi_close(env, dbi2);
    }
    mdb_env_close(env);

    env = 0;
    dbi1 = 0;
    dbi2 = 0;
    txn = 0;
}

void
addtodb(size_t db_nr, s8 key, s8 val)
{
    MDB_dbi dbi = db_nr == 1 ? dbi1 : dbi2;
    MDB_val key_s = { .mv_data = key.s, .mv_size = (size_t)key.len };
    MDB_val val_s = { .mv_data = val.s, .mv_size = (size_t)val.len };

    /* printf("Adding key: %s with value: %s\n\n", key, val); */
    rc = mdb_put(txn, dbi, &key_s, &val_s, 0);
    if (rc != MDB_KEYEXIST)
	MDB_CHECK(rc);
}

s8*
getfiles(s8 headw)
{
    MDB_CHECK(mdb_txn_begin(env, NULL, MDB_RDONLY, &txn));
    MDB_CHECK(mdb_dbi_open(txn, "head-file", MDB_DUPSORT, &dbi1));

    s8* ret = 0;

    MDB_val key_m = (MDB_val) { .mv_data = headw.s, .mv_size = (size_t)headw.len };
    MDB_val val_m = { 0 };

    MDB_cursor *cursor = 0;
    MDB_CHECK(mdb_cursor_open(txn, dbi1, &cursor));

    bool first = true;
    while ((rc = mdb_cursor_get(cursor, &key_m, &val_m, first ? MDB_SET_KEY : MDB_NEXT_DUP)) == 0)
    {
	s8 val = s8dup((s8){ .s = val_m.mv_data, .len = val_m.mv_size });
	buf_push(ret, val);

	first = 0;
    }
    if (rc != MDB_NOTFOUND)
	MDB_CHECK(rc);

    mdb_dbi_close(env, dbi1);
    mdb_txn_abort(txn);
    return ret;
}

s8
getreading(s8 fn)
{
    MDB_CHECK(mdb_txn_begin(env, NULL, MDB_RDONLY, &txn));
    MDB_CHECK(mdb_dbi_open(txn, "file-info", MDB_DUPSORT, &dbi2));

    MDB_val key_m = (MDB_val) { .mv_data = fn.s, .mv_size = (size_t)fn.len };
    MDB_val val_m = { 0 };

    if ((rc = mdb_get(txn, dbi2, &key_m, &val_m)) == MDB_NOTFOUND)
	return (s8){ 0 }
    ;
    MDB_CHECK(rc);

    mdb_dbi_close(env, dbi2);
    mdb_txn_abort(txn);
    return s8dup((s8){ .s = val_m.mv_data, .len = val_m.mv_size });
}
