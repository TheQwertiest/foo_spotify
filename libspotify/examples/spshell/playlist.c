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

#ifdef WIN32
#define alloca _alloca
#else
#include <alloca.h>
#endif

static int subscriptions_updated;

/**
 *
 */
int cmd_update_subscriptions(int argc, char **argv)
{
	int i;
	sp_playlistcontainer *pc = sp_session_playlistcontainer(g_session);
	sp_playlist *pl;
	subscriptions_updated = 1;
	for (i = 0; i < sp_playlistcontainer_num_playlists(pc); ++i) {
		switch (sp_playlistcontainer_playlist_type(pc, i)) {
		case SP_PLAYLIST_TYPE_PLAYLIST:
			pl = sp_playlistcontainer_playlist(pc, i);
			sp_playlist_update_subscribers(g_session, pl);
			break;
		default:
			break;
		}
	}
	return 1;
}



/**
 *
 */
int cmd_playlists(int argc, char **argv)
{
	sp_playlistcontainer *pc = sp_session_playlistcontainer(g_session);
	int i, j, level = 0;
	sp_playlist *pl;
	char name[200];
	int new = 0;

	printf("%d entries in the container\n", sp_playlistcontainer_num_playlists(pc));

	for (i = 0; i < sp_playlistcontainer_num_playlists(pc); ++i) {
		switch (sp_playlistcontainer_playlist_type(pc, i)) {
			case SP_PLAYLIST_TYPE_PLAYLIST:
				printf("%d. ", i);
				for (j = level; j; --j) printf("\t");
				pl = sp_playlistcontainer_playlist(pc, i);
				printf("%s", sp_playlist_name(pl));
				if(subscriptions_updated)
					printf(" (%d subscribers)", sp_playlist_num_subscribers(pl));

				new = sp_playlistcontainer_get_unseen_tracks(pc, pl, NULL, 0);
				if (new)
					printf(" (%d new)", new);

				printf("\n");
				break;
			case SP_PLAYLIST_TYPE_START_FOLDER:
				printf("%d. ", i);
				for (j = level; j; --j) printf("\t");
				sp_playlistcontainer_playlist_folder_name(pc, i, name, sizeof(name));
				printf("Folder: %s with id %llu\n", name,
					   sp_playlistcontainer_playlist_folder_id(pc, i));
				level++;
				break;
			case SP_PLAYLIST_TYPE_END_FOLDER:
				level--;
 				printf("%d. ", i);
				for (j = level; j; --j) printf("\t");
				printf("End folder with id %llu\n",	sp_playlistcontainer_playlist_folder_id(pc, i));
				break;
			case SP_PLAYLIST_TYPE_PLACEHOLDER:
				printf("%d. Placeholder", i);
				break;
		}
	}
	return 1;
}

/**
 *
 */
static void print_track2(sp_track *track, int i)
{

	printf("%d. %c %s%s %s\n", i,
	       sp_track_is_starred(g_session, track) ? '*' : ' ',
	       sp_track_is_local(g_session, track) ? "local" : "     ",
	       sp_track_is_autolinked(g_session, track) ? "autolinked" : "          ",
	       sp_track_name(track));
}


/**
 *
 */
int cmd_playlist(int argc, char **argv)
{
	int index, i, nnew;
	sp_playlist *playlist;
	sp_playlistcontainer *pc = sp_session_playlistcontainer(g_session);

	if (argc < 1) {
		printf("playlist [playlist index]\n");
		return 0;
	}

	index = atoi(argv[1]);
	if (index < 0 || index > sp_playlistcontainer_num_playlists(pc)) {
		printf("invalid index\n");
		return 0;
	}

	playlist = sp_playlistcontainer_playlist(pc, index);

	nnew = sp_playlistcontainer_get_unseen_tracks(pc, playlist, 0, 0);

	printf("Playlist %s by %s%s%s, %d new tracks\n",
		   sp_playlist_name(playlist),
		   sp_user_display_name(sp_playlist_owner(playlist)),
		   sp_playlist_is_collaborative(playlist) ? " (collaborative)" : "",
	           sp_playlist_has_pending_changes(playlist) ? " with pending changes" : "",
	           nnew
		   );

	if (argc == 3) {
		if (!strcmp(argv[2], "new")) {
			sp_track **tracks;

			if (nnew < 0)
				return 1;

			tracks = alloca(nnew * sizeof(*tracks));
			sp_playlistcontainer_get_unseen_tracks(pc, playlist, tracks, nnew);

			for (i = 0; i < nnew; i++) {
				print_track2(tracks[i], i);
			}
			return 1;

		} else if (!strcmp(argv[2], "clear-unseen"))
			sp_playlistcontainer_clear_unseen_tracks(pc, playlist);

	}
	for (i = 0; i < sp_playlist_num_tracks(playlist); ++i) {
		sp_track *track = sp_playlist_track(playlist, i);

		print_track2(track, i);
	}
	return 1;
}

/**
 *
 */
int cmd_set_autolink(int argc, char **argv)
{
	int index; 
	bool autolink;
	sp_playlist *playlist;
	sp_playlistcontainer *pc = sp_session_playlistcontainer(g_session);

	if (argc < 2) {
		printf("set autolink [playlist index] [0/1]\n");
		return 0;
	}

	index = atoi(argv[1]);
	autolink = atoi(argv[2]);
	if (index < 0 || index > sp_playlistcontainer_num_playlists(pc)) {
		printf("invalid index\n");
		return 0;
	}
	playlist = sp_playlistcontainer_playlist(pc, index);
	sp_playlist_set_autolink_tracks(playlist, !!autolink);
	printf("Set autolinking to %s on playlist %s\n", autolink ? "true": "false", sp_playlist_name(playlist));
	return 1;
}



/**
 *
 */
int cmd_add_folder(int argc, char **argv)
{
	int index; 
	const char *name;
	sp_playlistcontainer *pc = sp_session_playlistcontainer(g_session);

	if (argc < 2) {
		printf("addfolder [playlist index] [name]\n");
		return 0;
	}

	index = atoi(argv[1]);
	name = argv[2];

	if (index < 0 || index > sp_playlistcontainer_num_playlists(pc)) {
		printf("invalid index\n");
		return 0;
	}
	sp_playlistcontainer_add_folder(pc, index, name);
	return 1;
}


static sp_playlist_callbacks pl_update_callbacks;

/**
 *
 */
struct pl_update_work {
	int position;
	int num_tracks;
	sp_track **tracks;
};

static int apply_changes(sp_playlist *pl, struct pl_update_work *puw)
{
	sp_link *l;
	sp_error err;

	if(!sp_playlist_is_loaded(pl))
		return 1;


	l = sp_link_create_from_playlist(pl);
	if(l == NULL)
		return 1;
	sp_link_release(l);

	fprintf(stderr, "Playlist loaded, applying changes ... ");

	err = sp_playlist_add_tracks(pl, puw->tracks,
	    puw->num_tracks, puw->position, g_session);

	switch(err) {
	case SP_ERROR_OK:
		fprintf(stderr, "OK\n");
		break;
	case SP_ERROR_INVALID_INDATA:
		fprintf(stderr, "Invalid position\n");
		break;

	case SP_ERROR_PERMISSION_DENIED:
		fprintf(stderr, "Access denied\n");
		break;
	default:
		fprintf(stderr, "Other error (should not happen)\n");
		break;
	}
	return 0;
}

static void SP_CALLCONV pl_state_change(sp_playlist *pl, void *userdata)
{
	struct pl_update_work *puw = userdata;
	if(apply_changes(pl, puw))
		return;

	sp_playlist_remove_callbacks(pl, &pl_update_callbacks, puw);
	sp_playlist_release(pl);
	cmd_done();
}

static sp_playlist_callbacks pl_update_callbacks = {
	NULL,
	NULL,
	NULL,
	NULL,
	pl_state_change,
};


/**
 *
 */
int cmd_playlist_add_track(int argc, char **argv)
{
	sp_link *plink, *tlink;
	sp_track *t;
	sp_playlist *pl;
	int i;
	struct pl_update_work *puw;

	if(argc < 4) {
		printf("add [playlist uri] [position] [track uri] <[track uri]>...\n");
		return 1;
	}

	plink = sp_link_create_from_string(argv[1]);
	if (!plink) {
		fprintf(stderr, "%s is not a spotify link\n", argv[1]);
		return -1;
	}

	if(sp_link_type(plink) != SP_LINKTYPE_PLAYLIST) {
		fprintf(stderr, "%s is not a playlist link\n", argv[1]);
		sp_link_release(plink);
		return -1;
	}

	puw = malloc(sizeof(struct pl_update_work));
	puw->position = atoi(argv[2]);
	puw->tracks = malloc(sizeof(sp_track *) * argc - 3);
	puw->num_tracks = 0;
	for(i = 0; i < argc - 3; i++) {
		tlink = sp_link_create_from_string(argv[i + 3]);
		if(tlink == NULL) {
			fprintf(stderr, "%s is not a spotify link, skipping\n", argv[i + 3]);
			continue;
		}
		if(sp_link_type(tlink) != SP_LINKTYPE_TRACK) {
			fprintf(stderr, "%s is not a track link, skipping\n", argv[i + 3]);
			continue;
		}
		t = sp_link_as_track(tlink);
		sp_track_add_ref(t);
		puw->tracks[puw->num_tracks++] = t;
		sp_link_release(tlink);
	}

	pl = sp_playlist_create(g_session, plink);
	if(!apply_changes(pl, puw)) {
		// Changes applied directly, we're done
		sp_playlist_release(pl);
		sp_link_release(plink);
		return 1;
	}

	fprintf(stderr, "Playlist not yet loaded, waiting...\n");
	sp_playlist_add_callbacks(pl, &pl_update_callbacks, puw);
	sp_link_release(plink);
	return 0;
}

static const char *offlinestatus[] = {
	"None",         // SP_PLAYLIST_OFFLINE_STATUS_NO
	"Synchronized", // SP_PLAYLIST_OFFLINE_STATUS_YES
	"Downloading",  // SP_PLAYLIST_OFFLINE_STATUS_DOWNLOADING
	"Waiting",      // SP_PLAYLIST_OFFLINE_STATUS_WAITING
};


int cmd_playlist_offline(int argc, char **argv)
{
	sp_link *plink;
	sp_playlist *pl;
	int on;

	if (argc == 2 && !strcmp(argv[1], "status")) {
		printf("Offline status\n");
		printf("  %d tracks to sync\n",
		    sp_offline_tracks_to_sync(g_session));
		printf("  %d offline playlists in total\n",
		    sp_offline_num_playlists(g_session));
		return 1;
	}


	if (argc != 3) {
		printf("offline status | <playlist uri> <on|off>\n");
		return 1;
	}


	plink = sp_link_create_from_string(argv[1]);
	if (!plink) {
		fprintf(stderr, "%s is not a spotify link\n", argv[1]);
		return -1;
	}

	if (sp_link_type(plink) != SP_LINKTYPE_PLAYLIST) {
		fprintf(stderr, "%s is not a playlist link\n", argv[1]);
		sp_link_release(plink);
		return -1;
	}


	pl = sp_playlist_create(g_session, plink);

	if (argc == 3) {

		if (!strcasecmp(argv[2], "on"))
			on = 1;
		else if (!strcasecmp(argv[2], "off"))
			on = 0;
		else {
			fprintf(stderr, "Invalid mode: %s\n", argv[2]);
			return -1;
		}

		sp_playlist_set_offline_mode(g_session, pl, on);

	} else {

		sp_playlist_offline_status s;
		s = sp_playlist_get_offline_status(g_session, pl);

		printf("Offline status for %s (%s)\n",
		    argv[1], sp_playlist_name(pl));

		printf("  Status: %s\n", offlinestatus[s]);
		if (s == SP_PLAYLIST_OFFLINE_STATUS_DOWNLOADING)
			printf("    %d%% Complete\n",
			    sp_playlist_get_offline_download_completed(g_session, pl));
	}

	sp_playlist_release(pl);
	sp_link_release(plink);
	return 1;
}
