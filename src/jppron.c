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

#define DEFAULT_AUDIO_PATH "/home/daniel/.local/share/ajt_japanese_audio"

s8 curdir = { 0 };

static s8 buildpath(s8 a, s8 b)
{
#ifdef _WIN32
    s8 sep = s8("\\");
#else
    s8 sep = s8("/");
#endif
    size pathlen = a.len + sep.len + b.len;
    s8 path = news8(pathlen);
    s8 p = path;
    p = s8copy(p, a);
    p = s8copy(p, sep);
    p = s8copy(p, b);
    return path;
}

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

typedef struct {
    unsigned int with_o : 1;
} settings;


struct search_result {
    char** filenames;
    char** readings;
};

s8
build_audio_path(s8 indexdir, s8 audiofn)
{
#ifdef _WIN32
    s8 sep = s8("\\");
#else
    s8 sep = s8("/");
#endif
    s8 media = s8("media");

    size pathlen = indexdir.len + sep.len + media.len + sep.len + audiofn.len;
    s8 path = news8(pathlen);
    s8 p = path;
    p = s8copy(p, indexdir);
    p = s8copy(p, sep);
    p = s8copy(p, media);
    p = s8copy(p, sep);
    s8copy(p, audiofn);
    return path;
}

static void
add_filename(s8 headw, s8 fn)
{
    s8 fullpth = build_audio_path(curdir, fn);
    addtodb(1, headw, fullpth);
}

static void
add_fileinfo(s8 fn, s8 kana_reading)
{
    s8 fullpth = build_audio_path(curdir, fn);
    s8 hira_reading = kata2hira(kana_reading);
    addtodb(2, fullpth, hira_reading);
}

// wrapper for json api
s8
json_get_string_(json_stream* json)
{
    size_t slen = 0;
    s8 r = { 0 };
    r.s = (u8*)json_get_string(json, &slen);
    r.len = slen > 0 ? (size)(slen - 1) : 0;  // API includes terminating 0 in length
    return r;
}

static void
add_from_index(char* index_path)
{
    json_stream s[1];
    FILE* index = fopen(index_path, "r");
    if (!index)
	fatal_perror("Opening index file");
    json_open_stream(s, index);

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

	/* printf("type: %s, val: %.*s, depth: %li, head: %i\n\n", */
	/*        json_typename[type], (int)value.len, (char*)value.s, json_get_depth(s), */
	/*        reading_headwords ? 1 : 0); */

	if (type == JSON_ERROR)
	{
	    error_msg("%s", json_get_error(s));
	    break;
	}
	else if (type == JSON_DONE)
	    break;
	else if (!reading_headwords
		 && json_get_depth(s) <= 1
		 && type == JSON_STRING
		 && s8equals(value, s8("headwords")))
	{
	    reading_headwords = true;
	    assert(json_next(s) == JSON_OBJECT);
	    continue;
	}
	else if (reading_headwords && type == JSON_STRING)
	{
	    s8 headword = s8dup(value);
	    s8 fn = { 0 };

	    type = json_next(s);
	    if (type == JSON_STRING)
	    {
		fn = json_get_string_(s);
		add_filename(headword, fn);
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
			fn = json_get_string_(s);
			add_filename(headword, fn);
		    }
		    else
		    {
			error_msg("Encountered an unexpected type '%s', \
				in file name array.", json_typename[type]);
			json_skip_until(s, JSON_ARRAY_END);
		    }

		    type = json_next(s);
		}
	    }
	    else
		error_msg("Encountered unexpected type '%s' \
				for filename.", json_typename[type]);

	    free(headword.s);
	}
	else if (reading_headwords && type == JSON_OBJECT_END)
	    reading_headwords = false;
	else if (reading_headwords) // Warning: Order is important
	{
	    debug_msg("Skipping value of type '%s' while reading headwords.", json_typename[type]);
	    json_skip(s);
	}
	else if (!reading_headwords
		 && !reading_files
		 && json_get_depth(s) <= 1
		 && type == JSON_STRING
		 && s8equals(value, s8("files")))
	{
	    reading_files = true;
	    assert(json_next(s) == JSON_OBJECT);
	    continue;
	}
	else if (reading_files && type == JSON_OBJECT_END)
	    break;
	else if (reading_files && type == JSON_STRING)
	{
	    s8 fn = s8dup(value);
	    // TODO: Add check for audio filename ending (.ogg, .mp3, ...)
	    assert(json_next(s) == JSON_OBJECT);

	    type = json_next(s);
	    s8 prop = { 0 };
	    while (true)
	    {
		if (type == JSON_STRING)
		{
		    prop = json_get_string_(s);
		    if (s8equals(prop, s8("kana_reading")))
		    {
			json_next(s);
			s8 kana_reading = { 0 };
			kana_reading = json_get_string_(s);
			add_fileinfo(fn, kana_reading);
			break;
		    }
		}
		else if (type == JSON_ERROR)
		{
		    error_msg("%s", json_get_error(s));
		    break;
		}
		else if (type == JSON_OBJECT_END || type == JSON_DONE)
		{
		    debug_msg("Could not find 'kana_reading' for filename '%.*s'", (int)fn.len, (char*)fn.s);
		    break;
		}

		json_skip(s);
		type = json_next(s);
	    }

	    if (type != JSON_OBJECT_END)
		json_skip_until(s, JSON_OBJECT_END);

	    free(fn.s);
	}
    }

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
jppron_create(char* audio_dir_path, char* database_path)
{
    // TODO: Delete old db if existent

    if(create_dir(database_path))
	  fatal_perror("Creating directory");

    DIR* audio_dir = opendir(audio_dir_path);
    if (audio_dir == NULL)
	fatal_perror("Opening audio directory");

    opendb(database_path, false);

    struct dirent *entry;
    while ((entry = readdir(audio_dir)) != NULL)
    {
	if (strcmp(entry->d_name, ".") == 0
	    || strcmp(entry->d_name, "..") == 0)
	    continue;

	curdir = buildpath(fromcstr_(audio_dir_path), fromcstr_(entry->d_name));
	s8 index_path = buildpath(curdir, s8("index.json"));
	debug_msg("Processing path: %.*s", (int)curdir.len, (char*)curdir.s);

	if (access((char*)index_path.s, F_OK) == 0)
	    add_from_index((char*)index_path.s);
	else
	    debug_msg("No index file found");

	free(curdir.s);
	free(index_path.s);
    }

    closedb();
    closedir(audio_dir);
}

void
play_word(char* word, char* reading, char* database_path)
{
    s8 hira_reading = kata2hira(fromcstr_(reading));

    opendb(database_path, true);
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
	    s8 file_reading = getreading(files[i]);
	    if (s8equals(hira_reading, file_reading))
	    {
		play_audio((char*)files[i].s);
		match = true;
	    }
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
	    play_audio((char*)files[i].s);
    }

    frees8buffer(files);
    closedb();
}

char*
build_database_path()
{
    return g_build_filename(g_get_user_data_dir(), "jppron", NULL);
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
    char* dbpth = build_database_path();

    if (access(dbpth, R_OK) != 0)
    {
	msg("Indexing files..");
	jppron_create(audiopth, dbpth);
	msg("Index completed.");
    }

    play_word(word, reading, dbpth);

    g_free(dbpth);
}


#ifdef INCLUDE_MAIN
int
main(int argc, char** argv)
{
    if (argc < 2)
	fatal("Usage: %s word", argc > 0 ? argv[0] : "jppron");

    if (strcmp(argv[1], "-c") == 0)
	jppron_create(DEFAULT_AUDIO_PATH, build_database_path());
    else
	jppron(argv[1], argc > 2 ? argv[2] : 0, DEFAULT_AUDIO_PATH);
}
#endif
