/**
 * Copyright (c) 2006-2010 Spotify Ltd
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

#include <string.h>

#include "spshell.h"
#include "cmd.h"

sp_session *g_session;
void (*metadata_updated_fn)(void);
int is_logged_out;
int log_to_stderr;
int g_selftest;


/**
 * This callback is called when the user was logged in, but the connection to
 * Spotify was dropped for some reason.
 *
 * @sa sp_session_callbacks#connection_error
 */
static void SP_CALLCONV connection_error(sp_session *session, sp_error error)
{
	fprintf(stderr, "Connection to Spotify failed: %s\n",
	                sp_error_message(error));
}

/**
 * This callback is called when an attempt to login has succeeded or failed.
 *
 * @sa sp_session_callbacks#logged_in
 */
static void SP_CALLCONV logged_in(sp_session *session, sp_error error)
{
	sp_user *me;
	const char *display_name, *username;
	int cc;

	if (SP_ERROR_OK != error) {
		fprintf(stderr, "failed to log in to Spotify: %s\n",
		                sp_error_message(error));
		sp_session_release(session);
		exit(4);
	}

	// Let us print the nice message...
	me = sp_session_user(session);
	display_name = (sp_user_is_loaded(me) ? sp_user_display_name(me) : sp_user_canonical_name(me));
	username = sp_session_user_name (session);
	cc = sp_session_user_country(session);
	fprintf(stderr, "Logged in to Spotify as user %s [%s] (registered in country: %c%c)\n", username, display_name, cc >> 8, cc & 0xff);

#if WITH_TEST_COMMAND
	if(g_selftest)
		cmd_test(0, NULL);
	else
#endif
		start_prompt();
}

/**
 * This callback is called when the session has logged out of Spotify.
 *
 * @sa sp_session_callbacks#logged_out
 */
static void SP_CALLCONV logged_out(sp_session *session)
{
	is_logged_out = 1;  // Will exit mainloop
}

#ifdef SP_LIBSPOTIFY_WITH_SCROBBLING
/**
 * This callback is called when there is a scrobble error.
 *
 * @sa sp_session_callbacks#scrobble_error
 */
static void SP_CALLCONV scrobble_error(sp_session* session, sp_error error) {
	fprintf(stderr, "Scrobble failure: %d\n", error);
}

/**
 * Called when there is a change in the private session mode
 *
 * @param[in]  session    Session
 * @param[in]  isPrivate  True if in private session, false otherwhise
 */
static void SP_CALLCONV private_session_mode_changed(sp_session *session, bool is_private) {
	printf("private session mode changed: %d\n", is_private);
}
#endif

/**
 * This callback is called when the session have recieved a credential
 * that could be stored safely on disk
 *
 * @sa sp_session_callbacks#credentials_blob_updated
 */
static void SP_CALLCONV credentials_blob_updated(sp_session *session, const char *blob)
{
	printf("blob for storage: %s\n", blob);
}

/**
 * This callback is called for log messages.
 *
 * @sa sp_session_callbacks#log_message
 */
static void SP_CALLCONV log_message(sp_session *session, const char *data)
{
	if (log_to_stderr)
		fprintf(stderr, "%s", data);
}



/**
 * Callback called when libspotify has new metadata available
 *
 * Not used in this example (but available to be able to reuse the session.c file
 * for other examples.)
 *
 * @sa sp_session_callbacks#metadata_updated
 */
static void SP_CALLCONV metadata_updated(sp_session *sess)
{
	if(metadata_updated_fn)
		metadata_updated_fn();
#if WITH_TEST_COMMAND
	if(g_selftest)
		test_process();
#endif
}


/**
 *
 */
static void SP_CALLCONV offline_status_updated(sp_session *sess)
{
	sp_offline_sync_status status;
	sp_offline_sync_get_status(sess, &status);
	if(status.syncing) {
		printf("Offline status: queued:%d:%zd done:%d:%zd copied:%d:%zd nocopy:%d err:%d\n",
		    status.queued_tracks,
		    (size_t)status.queued_bytes,
		    status.done_tracks,
		    (size_t)status.done_bytes,
		    status.copied_tracks,
		    (size_t)status.copied_bytes,
		    status.willnotcopy_tracks,
		    status.error_tracks);
	} else {
		printf("Offline status: Idle\n");
	}
}

/**
 * Session callbacks
 */

static sp_session_callbacks callbacks;

/**
 *
 */
int spshell_init(const char *username, const char *password, const  char *blob, int selftest)
{
	sp_session_config config;
	sp_error error;
	sp_session *session;

        /// The application key is specific to each project, and allows Spotify
        /// to produce statistics on how our service is used.
	extern const char g_appkey[];
        /// The size of the application key.
	extern const size_t g_appkey_size;

	g_selftest = selftest;

	memset(&config, 0, sizeof(config));

	// Always do this. It allows libspotify to check for
	// header/library inconsistencies.
	config.api_version = SPOTIFY_API_VERSION;

	// The path of the directory to store the cache. This must be specified.
	// Please read the documentation on preferred values.
	config.cache_location = selftest ? "" : "tmp";

	// The path of the directory to store the settings. 
	// This must be specified.
	// Please read the documentation on preferred values.
	config.settings_location = selftest ? "" : "tmp";

	// The key of the application. They are generated by Spotify,
	// and are specific to each application using libspotify.
	config.application_key = g_appkey;
	config.application_key_size = g_appkey_size;

	// This identifies the application using some
	// free-text string [1, 255] characters.
	config.user_agent = "spshell";


	// Register the callbacks.
	callbacks.logged_in = logged_in;
	callbacks.logged_out = logged_out;
	callbacks.metadata_updated = metadata_updated;
	callbacks.connection_error = connection_error;
	callbacks.notify_main_thread = notify_main_thread;
#if WITH_TEST_COMMAND
	callbacks.music_delivery = music_delivery;
	callbacks.play_token_lost = play_token_lost;
	callbacks.end_of_track = end_of_track;
#endif
	callbacks.log_message = log_message;
	callbacks.offline_status_updated = offline_status_updated;
	callbacks.credentials_blob_updated = credentials_blob_updated;
#ifdef SP_LIBSPOTIFY_WITH_SCROBBLING
	callbacks.scrobble_error = scrobble_error;
	callbacks.private_session_mode_changed = private_session_mode_changed;
#endif
	config.callbacks = &callbacks;

	error = sp_session_create(&config, &session);
	if (SP_ERROR_OK != error) {
		fprintf(stderr, "failed to create session: %s\n",
		                sp_error_message(error));
		return 2;
	}

	// Login using the credentials given on the command line.
	if (username == NULL) {
		char reloginname[256];

		if (sp_session_relogin(session) == SP_ERROR_NO_CREDENTIALS) {
			fprintf(stderr, "No stored credentials\n");
			return 3;
		}
		sp_session_remembered_user(session, reloginname, sizeof(reloginname));
		fprintf(stderr, "Trying to relogin as user %s\n", reloginname);

	} else {
		sp_session_login(session, username, password, 1, blob);
	}

	g_session = session;
	return 0;
}


/**
 *
 */
int cmd_logout(int argc, char **argv)
{
	if(argc == 2 && !strcmp(argv[1], "permanent")) {
		fprintf(stderr, "Dropping stored credentials\n");
		sp_session_forget_me(g_session);
	}
	sp_session_logout(g_session);
	return 0;
}


/**
 *
 */
void test_finished(void)
{
	cmd_logout(0, NULL);
}

/**
 *
 */
int cmd_log(int argc, char **argv)
{
	if(argc != 2) {
		fprintf(stderr, "log enable|disable\n");
		return -1;
	}

	log_to_stderr = !strcmp(argv[1], "enable");
	return 1;
}
