#include <glib.h>
#include "util.h"

void
play_audio(char* filepath)
{
	g_autofree char* cmd = g_strdup_printf("ffplay -nodisp -nostats -hide_banner -autoexit '%s'", filepath);

	GError* error = NULL;
	g_spawn_command_line_sync(cmd, 0, 0, 0, &error);
	if (error)
	{
		error_msg("Failed to play file: %s. Error message: %s", filepath, error->message);
		g_error_free(error);
	}
}
