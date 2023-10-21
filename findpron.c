#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define MAX_LINE_LENGTH 100 // WARNING: breaks if file has line longer than this

// TODO: Deinflect

void
die(const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	if (fmt[0] && fmt[strlen(fmt)-1] == ':') {
		fputc(' ', stderr);
		perror(NULL);
	} else {
		fputc('\n', stderr);
	}

	exit(1);
}

void
close_exit(FILE * fp, int exit_code)
{
    fclose(fp);
    exit(exit_code);
}

char *
findlast(char *haystack, char needle)
{
  char *last;
  for (int i = 0; haystack[i] != '\0'; i++)
    if (haystack[i] == needle)
      last = haystack + i;
  return last;
}

void
playfile(char *indexpth, char *fn)
{
	char *cmd;
	asprintf(&cmd, "ffplay -fast -loglevel quiet -autoexit -nodisp %s/media/%s", indexpth, fn);
	printf("Executing command: %s\n", cmd);

	FILE *fp;
	fp = popen(cmd, "r");
	if (fp == NULL) die("Failed calling the above command.");
	pclose(fp);
	free(cmd);
}

/* Modifies input string */
char *
extract_filename(char *line)
{
	char *filename = strstr(line, "\"") + 1;
	int end = strstr(filename, "\"") - filename;
	filename[end] = '\0';
	return filename;
}

int
main(int argc, char**argv)
{
    if (argc < 3)
	die("Usage: %s path_to_index word [alt_word]", argv[0]);

    char *word = argv[2];
    char *wordKana = argc >= 4 ? argv[3] : NULL;

    char wordEx[MAX_LINE_LENGTH];
    char wordKanaEx[MAX_LINE_LENGTH];
    snprintf(wordEx, sizeof(wordEx), "\"%s\"", word);
    snprintf(wordKanaEx, sizeof(wordKanaEx), "\"%s\"", wordKana);

    FILE * fp;
    char line[MAX_LINE_LENGTH];

    fp = fopen(argv[1], "r");
    if (fp == NULL)
        die("Could not open file");

    *findlast(argv[1], '/') = '\0'; // strip filename

    /* skip to headwords */
    while (fgets(line, MAX_LINE_LENGTH, fp))
	  if(strstr(line, "\"headwords\"")) break;

    char *filename = NULL;
    while (fgets(line, MAX_LINE_LENGTH, fp))
    {
	  if (line[5] == 'f') // Reached "file" in json
	      break;

	  if (line[8] != '"')
	    continue;

	  if (strstr(line, wordEx))
	  {
	      fgets(line, MAX_LINE_LENGTH, fp);
	      for(;!(strstr(line, "]")); fgets(line, MAX_LINE_LENGTH, fp))
	      {
		  filename = extract_filename(line);
		  playfile(argv[1], filename);
	      }

	      close_exit(fp, EXIT_SUCCESS);
	  }

	  if (argc >= 4 && strstr(line, wordKanaEx))
	  {
	      /* Only plays first result */
	      fgets(line, MAX_LINE_LENGTH, fp);
	      filename = strdup(extract_filename(line)); // free?
	      /* Don't break -> prefer non kana */
	  }
    }
    if (filename)
    {
	  playfile(argv[1], filename);
	  close_exit(fp, EXIT_SUCCESS);
    }

    close_exit(fp, EXIT_FAILURE);
}
