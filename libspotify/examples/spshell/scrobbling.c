/**
 * Copyright (c) 2006-2012 Spotify Ltd
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "spshell.h"
#include <string.h>

void print_scrobbling_state(sp_scrobbling_state state) {
	if (state == SP_SCROBBLING_STATE_LOCAL_ENABLED)
		printf("locally overridden, enabled");
	else if (state == SP_SCROBBLING_STATE_LOCAL_DISABLED)
		printf("locally overridden, disabled");
	else if (state == SP_SCROBBLING_STATE_GLOBAL_ENABLED)
		printf("globally set, enabled");
	else if (state == SP_SCROBBLING_STATE_GLOBAL_DISABLED)
		printf("globally set, disabled");
	else
		printf("<unknown>");
}


int cmd_facebook_scrobbling(int argc, char **argv)
{
	if(argc != 2) {
		fprintf(stderr, "facebook scrobbling local override enable|disable|unset\n");
		return -1;
	}

	sp_scrobbling_state state = SP_SCROBBLING_STATE_USE_GLOBAL_SETTING;
	if (strcmp(argv[1], "unset")) {
		// not "unset"
		bool enable = !strcmp(argv[1], "enable");
		state = enable ? SP_SCROBBLING_STATE_LOCAL_ENABLED : SP_SCROBBLING_STATE_LOCAL_DISABLED;
	}
	sp_session_set_scrobbling(g_session, SP_SOCIAL_PROVIDER_FACEBOOK, state);

	return 1;
}

int cmd_is_facebook_scrobbling(int argc, char **argv)
{
	printf("facebook scrobbling state is ");
	sp_scrobbling_state state;
	sp_session_is_scrobbling(g_session, SP_SOCIAL_PROVIDER_FACEBOOK, &state);
	print_scrobbling_state(state);
	printf("\n");
	return 1;
}

int cmd_spotify_social(int argc, char **argv)
{
	if(argc != 2) {
		fprintf(stderr, "spotify social local override enable|disable|unset\n");
		return -1;
	}

	sp_scrobbling_state state = SP_SCROBBLING_STATE_USE_GLOBAL_SETTING;
	if (strcmp(argv[1], "unset")) {
		// not "unset"
		bool enable = !strcmp(argv[1], "enable");
		state = enable ? SP_SCROBBLING_STATE_LOCAL_ENABLED : SP_SCROBBLING_STATE_LOCAL_DISABLED;
	}
	sp_session_set_scrobbling(g_session, SP_SOCIAL_PROVIDER_SPOTIFY, state);

	return 1;
}

int cmd_is_spotify_social(int argc, char **argv)
{
	printf("spotify social scrobbling state is ");
	sp_scrobbling_state state;
	sp_session_is_scrobbling(g_session, SP_SOCIAL_PROVIDER_SPOTIFY, &state);
	print_scrobbling_state(state);
	printf("\n");
	return 1;
}

int cmd_set_lastfm_scrobbling_credentials(int argc, char **argv)
{
	if(argc != 3) {
		fprintf(stderr, "lastfm_scrobbling_credentials username password\n");
		return -1;
	}

	sp_session_set_social_credentials(g_session, SP_SOCIAL_PROVIDER_LASTFM, argv[1], argv[2]);
	return 1;
}

int cmd_lastfm_scrobbling(int argc, char **argv)
{
	if(argc != 2) {
		fprintf(stderr, "lastfm_scrobbling enable|disable\n");
		return -1;
	}

	bool enable = !strcmp(argv[1], "enable");
	sp_scrobbling_state state = enable ? SP_SCROBBLING_STATE_LOCAL_ENABLED : SP_SCROBBLING_STATE_LOCAL_DISABLED;

	sp_session_set_scrobbling(g_session, SP_SOCIAL_PROVIDER_LASTFM, state);
	return 1;
}

int cmd_is_lastfm_scrobbling(int argc, char **argv)
{
	printf("lastfm scrobbling state is ");
	sp_scrobbling_state state;
	sp_session_is_scrobbling(g_session, SP_SOCIAL_PROVIDER_LASTFM, &state);

	if (state == SP_SCROBBLING_STATE_LOCAL_ENABLED)
		printf("enabled");
	else if (state == SP_SCROBBLING_STATE_LOCAL_DISABLED)
		printf("disabled");
	else
		printf("<unknown>");

	printf("\n");
	return 1;
}

extern int cmd_private_session(int argc, char** argv)
{
	if(argc != 2) {
		fprintf(stderr, "private_session enable|disable\n");
		return -1;
	}

	sp_session_set_private_session(g_session, !strcmp(argv[1], "enable"));
	return 1;
}

extern int cmd_is_private_session(int argc, char** argv)
{
	printf("private_session is ");
	printf( sp_session_is_private_session(g_session) ? "enabled" : "disabled");
	printf("\n");
	return 1;
}

