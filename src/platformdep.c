#include <glib.h>
#include "util.h"

void
play_audio(int len, char filepath[len])
{
	g_autofree char* cmd = g_strdup_printf("ffplay -nodisp -nostats -hide_banner -autoexit '%.*s'", len, filepath);

	GError* error = NULL;
	g_spawn_command_line_sync(cmd, 0, 0, 0, &error);
	if (error)
	{
		error_msg("Failed to play file: %.*s. Error message: %s", len, filepath, error->message);
		g_error_free(error);
	}
}
