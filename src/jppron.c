#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h> // access
#include <alloca.h>
#include <sys/stat.h> // mkdir

#include <glib.h>

#include "pdjson.h"
#include "database.h"
#include "deinflector.h"
#include "util.h"
#include "platformdep.h"

// For access()
#ifdef _WIN32
#include <io.h>
#define F_OK 0
#define access _access
#endif

typedef struct {
    s8 origin;
    s8 hira_reading;
    s8 pitch_number;
    s8 pitch_pattern;
} fileinfo;

const char json_typename[][16] = {
    [JSON_ERROR] = "ERROR",
    [JSON_DONE] = "DONE",
    [JSON_OBJECT] = "OBJECT",
    [JSON_OBJECT_END] = "OBJECT_END",
    [JSON_ARRAY] = "ARRAY",
    [JSON_ARRAY_END] = "ARRAY_END",
    [JSON_STRING] = "STRING",
    [JSON_NUMBER] = "NUMBER",
    [JSON_TRUE] = "TRUE",
    [JSON_FALSE] = "FALSE",
    [JSON_NULL] = "NULL",
};


static void
print_fileinfo(fileinfo fi)
{
    printf("Source: %.*s\nReading: %.*s\nPitch number: %.*s\nPitch pattern: %.*s\n",
	   (int)fi.origin.len, (char*)fi.origin.s,
	   (int)fi.hira_reading.len, (char*)fi.hira_reading.s,
	   (int)fi.pitch_number.len, (char*)fi.pitch_number.s,
	   (int)fi.pitch_pattern.len, (char*)fi.pitch_pattern.s);
}

static void
freefileinfo(fileinfo fi[static 1])
{
    /* frees8(&fi->origin); */
    frees8(&fi->hira_reading);
    frees8(&fi->pitch_number);
    frees8(&fi->pitch_pattern);
}

static s8
build_audio_path(s8 indexdir, s8 audiofn)
{
    return buildpath(indexdir, s8("media"), audiofn);
}

static void
prints8(s8 z)
{
    for (int i = 0; i < z.len; i++)
	putchar((int)z.s[i]);
    putchar('\n');
}

static void
add_filename(s8 headw, s8 fullpth)
{
    addtodb1(headw, fullpth);
}

static void
add_fileinfo(s8 fullpth, fileinfo fi)
{
    s8 sep = s8("\0");
    s8 data = s8concat(fi.origin, sep, fi.hira_reading, sep, fi.pitch_number, sep, fi.pitch_pattern);
    addtodb2(fullpth, data);
    frees8(&data);
}

// wrapper for json api
static s8
json_get_string_(json_stream* json)
{
    size_t slen = 0;
    s8 r = { 0 };
    r.s = (u8*)json_get_string(json, &slen);
    r.len = slen > 0 ? (size)(slen - 1) : 0;  // API includes terminating 0 in length
    return r;
}

static void
add_from_index(char* index_path, s8 curdir)
{
    FILE* index = fopen(index_path, "r");
    if (!index)
	fatal_perror("Opening index file");
    json_stream s[1];
    json_open_stream(s, index);

    s8 cursrc = { 0 };
    s8 mediadir = { 0 };

    bool reading_meta = false;
    bool reading_headwords = false;
    bool reading_files = false;

    enum json_type type;
    for (;;)
    {
	type = json_next(s);
	s8 value = { 0 };
	switch (type)
	{
	case JSON_NULL:
	    value = s8("null");
	    break;
	case JSON_TRUE:
	    value = s8("true");
	    break;
	case JSON_FALSE:
	    value = s8("false");
	    break;
	case JSON_NUMBER:
	    value = json_get_string_(s);
	    break;
	case JSON_STRING:
	    value = json_get_string_(s);
	default:
	    break;
	}

	if (type == JSON_ERROR)
	{
	    error_msg("%s", json_get_error(s));
	    break;
	}
	else if (type == JSON_DONE)
	    break;

	if (!reading_meta
	    && json_get_depth(s) == 1
	    && type == JSON_STRING
	    && s8equals(value, s8("meta")))
	{
	    reading_meta = true;
	    type = json_next(s);
	    assert(type == JSON_OBJECT);
	}
	else if (reading_meta
		 && json_get_depth(s) == 1
		 && type == JSON_OBJECT_END)
	{
	    reading_meta = false;
	}
	else if (reading_meta)
	{
	    if (type == JSON_STRING)
	    {
		value = json_get_string_(s);
		if (s8equals(value, s8("name")))
		{
		    type = json_next(s);
		    assert(type == JSON_STRING);
		    cursrc = s8dup(json_get_string_(s));
		}
		else if (s8equals(value, s8("media_dir")))
		{
		    type = json_next(s);
		    assert(type == JSON_STRING);
		    mediadir = s8dup(json_get_string_(s));
		}
		else
		    json_skip(s);
	    }
	    else
		json_skip(s);
	}
	else if (!reading_headwords
		 && json_get_depth(s) == 1
		 && type == JSON_STRING
		 && s8equals(value, s8("headwords")))
	{
	    reading_headwords = true;
	    type = json_next(s);
	    assert(type == JSON_OBJECT);
	}
	else if (reading_headwords
		 && json_get_depth(s) == 1
		 && type == JSON_OBJECT_END)
	{
	    reading_headwords = false;
	}
	else if (reading_headwords && type == JSON_STRING)
	{
	    s8 headword = s8dup(value);

	    type = json_next(s);
	    if (type == JSON_STRING)
	    {
		s8 fn = json_get_string_(s);
		s8 fullpth = buildpath(curdir, mediadir, fn);
		add_filename(headword, fullpth);
		frees8(&fullpth);
	    }
	    else if (type == JSON_ARRAY)
	    {
		type = json_next(s);
		while (type != JSON_ARRAY_END
		       && type != JSON_DONE
		       && type != JSON_ERROR)
		{
		    if (type == JSON_STRING)
		    {
			s8 fn = json_get_string_(s);
			s8 fullpth = buildpath(curdir, mediadir, fn);
			add_filename(headword, fullpth);
			frees8(&fullpth);
		    }
		    else
			error_msg("Encountered an unexpected type '%s', \
				in file name array.", json_typename[type]);

		    type = json_next(s);
		}
	    }
	    else
		error_msg("Encountered unexpected type '%s' \
				for filename of headword '%.*s'.",
			  json_typename[type], (int)headword.len, (char*)headword.s);

	    frees8(&headword);
	}
	else if (reading_headwords) // Warning: Order is important
	{
	    debug_msg("Skipping entry of type '%s' while reading headwords.", json_typename[type]);
	    json_skip(s);
	}
	else if (!reading_files
		 && json_get_depth(s) == 1
		 && type == JSON_STRING
		 && s8equals(value, s8("files")))
	{
	    reading_files = true;
	    type = json_next(s);
	    assert(type == JSON_OBJECT);
	}
	else if (reading_files
		 && json_get_depth(s) == 1
		 && type == JSON_OBJECT_END)
	{
	    reading_files = false;
	    break;
	}
	else if (reading_files && type == JSON_STRING)
	{
	    // TODO: Add debug check for audio filename ending (.ogg, .mp3, ...)
	    s8 fn = value;
	    s8 fullpth = buildpath(curdir, mediadir, fn);

	    type = json_next(s);
	    assert(type == JSON_OBJECT);
	    type = json_next(s);
	    fileinfo fi = { .origin = cursrc };
	    while (type != JSON_OBJECT_END && type != JSON_DONE)
	    {
		if (type == JSON_STRING)
		{
		    value = json_get_string_(s);
		    if (s8equals(value, s8("kana_reading")))
		    {
			type = json_next(s);
			assert(type == JSON_STRING);
			fi.hira_reading = kata2hira(json_get_string_(s));
		    }
		    else if (s8equals(value, s8("pitch_number")))
		    {
			type = json_next(s);
			assert(type == JSON_STRING);
			fi.pitch_number = s8dup(json_get_string_(s));
		    }
		    else if (s8equals(value, s8("pitch_pattern")))
		    {
			type = json_next(s);
			assert(type == JSON_STRING);
			fi.pitch_pattern = s8dup(json_get_string_(s));
		    }
		    else
			json_skip(s);
		}
		else if (type == JSON_ERROR)
		{
		    error_msg("%s", json_get_error(s));
		    break;
		}
		else
		    json_skip(s);

		type = json_next(s);
	    }
	    if (type != JSON_OBJECT_END)
		json_skip_until(s, JSON_OBJECT_END);

	    add_fileinfo(fullpth, fi);
	    freefileinfo(&fi);
	    frees8(&fullpth);
	}
	else if (reading_files)
	{
	    debug_msg("Skipping entry of type '%s' while reading files.", json_typename[type]);
	    json_skip(s);
	}
    }

    frees8(&cursrc);
    fclose(index);
    json_close(s);
}

/*
 * Returns: 0 on success, -1 on failure and sets errno
 */
int
create_dir(char* dir_path)
{
    //TODO: Recursive implementation. Maybe like this:
    /* char tmp[256]; */
    /* char *p = NULL; */
    /* size_t len; */

    /* snprintf(tmp, sizeof(tmp),"%s",dir); */
    /* len = strlen(tmp); */
    /* if (tmp[len - 1] == '/') */
    /*     tmp[len - 1] = 0; */
    /* for (p = tmp + 1; *p; p++) */
    /*     if (*p == '/') { */
    /*         *p = 0; */
    /*         mkdir(tmp, S_IRWXU); */
    /*         *p = '/'; */
    /*     } */
    /* mkdir(tmp, S_IRWXU); */

    int status = mkdir(dir_path, S_IRWXU | S_IRWXG | S_IXOTH);
    return (status == 0 || errno == EEXIST) ? 0 : -1;
}

void
jppron_create(char* audio_dir_path, s8 database_path)
{
    // TODO: Delete old db if existent
    if (create_dir((char*)database_path.s))
	fatal_perror("Creating directory");

    DIR* audio_dir;
    if ((audio_dir = opendir(audio_dir_path)) == NULL)
	fatal_perror("Opening audio directory");

    opendb((char*)database_path.s, false);

    struct dirent *entry;
    while ((entry = readdir(audio_dir)) != NULL)
    {
	if (strcmp(entry->d_name, ".") == 0
	    || strcmp(entry->d_name, "..") == 0)
	    continue;

	s8 curdir = buildpath(fromcstr_(audio_dir_path), fromcstr_(entry->d_name));
	s8 index_path = buildpath(curdir, s8("index.json"));
	debug_msg("Processing path: %.*s", (int)curdir.len, (char*)curdir.s);

	if (access((char*)index_path.s, F_OK) == 0)
	    add_from_index((char*)index_path.s, curdir);
	else
	    debug_msg("No index file found");

	frees8(&curdir);
	frees8(&index_path);
    }

    closedb();
    closedir(audio_dir);

    s8 lock_file = buildpath(database_path, s8("lock.mdb"));
    remove((char*)lock_file.s);
    frees8(&lock_file);
}

static fileinfo
getfileinfo(s8 fn)
{
    s8 data = getfromdb2(fn);

    s8 d = data;

    s8 data_split[4];
    for (int i = 0; i < 3; i++)
    {
	data_split[i] = s8dup(fromcstr_((char*)d.s));

	d.s += data_split[i].len + 1;
	d.len -= data_split[i].len + 1;
	assert(d.len >= 0);
    }
    data_split[3] = s8dup(d);

    frees8(&data);

    return (fileinfo){
	       .origin = data_split[0],
	       .hira_reading = data_split[1],
	       .pitch_number = data_split[2],
	       .pitch_pattern = data_split[3]
    };
}

static void
play_word(char* word, char* reading, s8 database_path)
{
    s8 hira_reading = kata2hira(fromcstr_(reading));

    opendb((char*)database_path.s, true);
    s8* files = getfiles(fromcstr_(word));

    if (!files)
    {
	msg("Nothing found.");
	closedb();
	return;
    }

    if (reading)
    {
	bool match = false;
	for (size_t i = 0; i < buf_size(files); i++)
	{
	    fileinfo fi = getfileinfo(files[i]);
	    if (s8equals(hira_reading, fi.hira_reading))
	    {
		print_fileinfo(fi);
		play_audio(files[i].len, (char*)files[i].s);
		match = true;
	    }
	    freefileinfo(&fi);
	}
	if (!match)
	{
	    msg("Could not find an audio file with corresponding reading. Playing all..");
	    reading = 0;
	}
    }
    if (!reading)
    {
	// Play all
	for (size_t i = 0; i < buf_size(files); i++)
	{
	    fileinfo fi = getfileinfo(files[i]);
	    print_fileinfo(fi);
	    freefileinfo(&fi);

	    play_audio(files[i].len, (char*)files[i].s);
	}
    }

    frees8buffer(files);
    closedb();
}

static s8
build_database_path()
{
    return fromcstr_(g_build_filename(g_get_user_data_dir(), "jppron", NULL));
}

/**
 * jppron:
 * @word: word to be pronounced
 * @audiopth: (optional): Path to the ajt-style audio file directories
 *
 */
void
jppron(char* word, char* reading, char* audiopth)
{
    s8 dbpth = build_database_path();

    s8 dbfile = buildpath(dbpth, s8("data.mdb"));
    int no_access = access((char*)dbfile.s, R_OK);
    frees8(&dbfile);

    if (no_access)
    {
	if (audiopth)
	{
	    msg("Indexing files..");
	    jppron_create(audiopth, dbpth); // TODO: エラー処理
	    msg("Index completed.");
	}
	else
	{
	    debug_msg("No (readable) database file and no audio path provided. Exiting..");
	    return;
	}
    }

    play_word(word, reading, dbpth);

    frees8(&dbpth);
}

#ifdef INCLUDE_MAIN
int
main(int argc, char** argv)
{
    if (argc < 2)
	fatal("Usage: %s word", argc > 0 ? argv[0] : "jppron");

    char* default_audio_path = g_build_filename(g_get_user_data_dir(), "ajt_japanese_audio", NULL);

    if (strcmp(argv[1], "-c") == 0)
	jppron_create(default_audio_path, build_database_path());
    else
	jppron(argv[1], argc > 2 ? argv[2] : 0, default_audio_path);
}
#endif
