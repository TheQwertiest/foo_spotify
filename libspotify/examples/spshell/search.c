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


/**
 * Print the given album metadata
 *
 * @param  album  The album object
 */
static void print_album(sp_album *album)
{
	printf("  Album \"%s\" (%d)\n",
	       sp_album_name(album),
	       sp_album_year(album));
}

/**
 * Print the given artist metadata
 *
 * @param  artist  The artist object
 */
static void print_artist(sp_artist *artist)
{
	printf("  Artist \"%s\"\n", sp_artist_name(artist));
}

/**
 * Print the given search result with as much information as possible
 *
 * @param  search   The search result
 */
static void print_search(sp_search *search)
{
	int i;

	printf("Query          : %s\n", sp_search_query(search));
	printf("Did you mean   : %s\n", sp_search_did_you_mean(search));
	printf("Tracks in total: %d\n", sp_search_total_tracks(search));
	puts("");

	for (i = 0; i < sp_search_num_tracks(search); ++i)
		print_track(sp_search_track(search, i));

	puts("");

	for (i = 0; i < sp_search_num_albums(search); ++i)
		print_album(sp_search_album(search, i));

	puts("");

	for (i = 0; i < sp_search_num_artists(search); ++i)
		print_artist(sp_search_artist(search, i));

	puts("");

	for (i = 0; i < sp_search_num_playlists(search); ++i) {
		// print some readily available metadata, the rest will
		// be available from the sp_playlist object loaded through
		// sp_search_playlist().
		printf("  Playlist \"%s\"\n", sp_search_playlist_name(search, i));
	}
}

/**
 * Callback for libspotify
 *
 * @param browse    The browse result object that is now done
 * @param userdata  The opaque pointer given to sp_artistbrowse_create()
 */
static void SP_CALLCONV search_complete(sp_search *search, void *userdata)
{
	if (sp_search_error(search) == SP_ERROR_OK)
		print_search(search);
	else
		fprintf(stderr, "Failed to search: %s\n",
		        sp_error_message(sp_search_error(search)));

	sp_search_release(search);
	cmd_done();
}



/**
 *
 */
static void search_usage(void)
{
	fprintf(stderr, "Usage: search <query>\n");
}


/**
 *
 */
int cmd_search(int argc, char **argv)
{
	char query[1024];
	int i;

	if (argc < 2) {
		search_usage();
		return -1;
	}

	query[0] = 0;
	for(i = 1; i < argc; i++)
		snprintf(query + strlen(query), sizeof(query) - strlen(query), "%s%s",
			 i == 1 ? "" : " ", argv[i]);

	sp_search_create(g_session, query, 0, 100, 0, 100, 0, 100, 0, 100, SP_SEARCH_STANDARD, &search_complete, NULL);
	return 0;
}


/**
 *
 */
int cmd_whatsnew(int argc, char **argv)
{
	sp_search_create(g_session, "tag:new", 0, 0, 0, 250, 0, 0, 0, 0, SP_SEARCH_STANDARD, &search_complete, NULL);
	return 0;
}
