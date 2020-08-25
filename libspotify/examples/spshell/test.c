/**
 * Copyright (c) 2006-2011 Spotify Ltd
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
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#if __APPLE__ || __linux__
#include <sys/resource.h>
#endif // __APPLE__ || __linux__

#include "spshell.h"
#include "cmd.h"

#define STRINGIFY(s) TOSTRING(s)
#define TOSTRING(s) #s



enum msglevel {
	MSG_ERROR,
	MSG_INFO,
	MSG_OK,
};

#if WIN32
const char *dec_by_level[] = {
	"",
	"",
	""
};
static const char *decorate_clr = "";
#else
const char *dec_by_level[] = {
	"\033[31m",
	"\033[36m",
	"\033[32m"
};
static const char *decorate_clr   = "\033[0m";
#endif

typedef struct Test {
	// Initialized by TEST_DECL
	const char *title;
	int maxtime;
	int arg1;
	int arg2;
	int arg3;

	// Other members
	struct Test *next;
	sp_uint64 start;
	int duration;
	int done;
} Test;

static struct Test *active_tests;




#define TEST_DECL(title,maxtime) Test title = { STRINGIFY(title), maxtime }
#define TEST_DECL3(title,maxtime, a) Test title = { STRINGIFY(title), maxtime, a }
#define TEST_DECL4(title,maxtime, a,b) Test title = { STRINGIFY(title), maxtime, a,b }
#define TEST_DECL5(title,maxtime, a,b,c) Test title = { STRINGIFY(title), maxtime, a,b,c }

void test_activate(struct Test *t)
{
	t->next = active_tests;
	active_tests = t;
}


void test_list_active(void)
{
	struct Test *t;
	for(t = active_tests; t != NULL; t = t->next)
		printf("%s\n", t->title);
}


void output(enum msglevel level, const char *fmt, ...)
{
	char msg[512];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(msg, sizeof(msg), fmt, ap);
	va_end(ap);

	printf("%s%s%s\n", dec_by_level[level], msg, decorate_clr);

}

void test_deactivate(const struct Test * const t)
{
	struct Test *l, **p = &active_tests;

	while((l = *p) != NULL) {
		if(t == l) {
			*p = l->next;
			return;
		}
		p = &l->next;
	}
	output(MSG_ERROR, "Test %s not linked", t->title);
	abort();
}


static int test_done(Test *t, int backend_time)
{
	t->done++;

	if(t->done > 1) {
		output(MSG_ERROR, "%s: Finished multiple times (%d)", t->title, t->done);
		abort();
		return 1;
	}

	test_deactivate(t);

	t->duration = (int)(get_ts() - t->start);

	if(t->duration > t->maxtime * 1000) {
		char trailer[100];

		if(backend_time != -1) {
			snprintf(trailer, sizeof(trailer), ", %dms waiting for backend",
				 backend_time);
		} else {
			trailer[0] = 0;
		}
		output(MSG_ERROR, "%s: %d Time exceeded, limit is %dms%s",
		       t->title, t->duration, t->maxtime * 1000, trailer);
		return 1;
	}
	return 0;
}


static void test_ok(Test *t)
{
	if(test_done(t, -1))
		return;

	output(MSG_OK, "%s: %d OK", t->title, t->duration);
}


static void test_ok_backend_duration(Test *t, int backend_duration)
{
	char trailer[100];

	if(test_done(t, backend_duration))
		return;

	if(backend_duration == -1)
		strcpy(trailer, "Loaded from cache");
	else
		snprintf(trailer, sizeof(trailer), "%d ms backend request delay", backend_duration);

	output(MSG_OK, "%s: %d OK, %s", t->title, t->duration, trailer);
}

static void test_report(Test *t, const char *fmt, ...)
{
	char msg[512];
	va_list ap;
	int level;

	if(test_done(t, -1))
		return;

	level = MSG_ERROR;
	va_start(ap, fmt);
	vsnprintf(msg, sizeof(msg), fmt, ap);
	va_end(ap);

	output(level, "%s: %d %s", t->title, t->duration, msg);
}


#define info_report(fmt, ...) output(MSG_INFO, fmt, ##__VA_ARGS__)


static void test_start(Test *t)
{
	t->start = get_ts();
	test_activate(t);
}

/*****************************************************************
 * URI helpers
 */
static sp_artist *artist_from_uri(const char *uri)
{
	sp_artist *a;
	sp_link *l = sp_link_create_from_string(uri);
	if(sp_link_type(l) != SP_LINKTYPE_ARTIST) {
		sp_link_release(l);
		return NULL;
	}
	a = sp_link_as_artist(l);
	sp_artist_add_ref(a);
	sp_link_release(l);
	return a;
}


static sp_album *album_from_uri(const char *uri)
{
	sp_album *a;
	sp_link *l = sp_link_create_from_string(uri);
	if(sp_link_type(l) != SP_LINKTYPE_ALBUM) {
		sp_link_release(l);
		return NULL;
	}
	a = sp_link_as_album(l);
	sp_album_add_ref(a);
	sp_link_release(l);
	return a;
}


static sp_track *track_from_uri(const char *uri)
{
	sp_track *t;
	sp_link *l = sp_link_create_from_string(uri);
	if(sp_link_type(l) != SP_LINKTYPE_TRACK) {
		sp_link_release(l);
		return NULL;
	}
	t = sp_link_as_track(l);
	sp_track_add_ref(t);
	sp_link_release(l);
	return t;
}


/*****************************************************************
 * Search
 */
TEST_DECL(search1, 3);
TEST_DECL(search2, 3);
TEST_DECL(did_you_mean, 3);


static void SP_CALLCONV search1_cb(sp_search *result, void *userdata)
{
	if(!sp_search_is_loaded(result))
		test_report(userdata, "Result not loaded");
	else if(sp_search_error(result) != SP_ERROR_OK)
		test_report(userdata, "%s", sp_error_message(sp_search_error(result)));
	else if(sp_search_num_tracks(result) != 2)
		test_report(userdata, "Expected %d tracks got %d", 2, sp_search_num_tracks(result));
	else if(sp_search_total_tracks(result) < sp_search_num_tracks(result))
		test_report(userdata, "Total tracks (%d) less than number of tracks (%d)", sp_search_total_tracks(result), sp_search_num_tracks(result));
	else if(sp_search_num_albums(result) != 4)
		test_report(userdata, "Expected %d albums got %d", 4, sp_search_num_albums(result));
	else if(sp_search_total_albums(result) < sp_search_num_albums(result))
		test_report(userdata, "Total albums (%d) less than number of albums (%d)", sp_search_total_albums(result), sp_search_num_albums(result));
	else if(sp_search_num_artists(result) != 6)
		test_report(userdata, "Expected %d artists got %d", 6, sp_search_num_artists(result));
	else if(sp_search_total_artists(result) < sp_search_num_artists(result))
		test_report(userdata, "Total artists (%d) less than number of artists (%d)", sp_search_total_artists(result), sp_search_num_artists(result));
	else if(sp_search_num_playlists(result) != 8)
		test_report(userdata, "Expected %d playlists got %d", 8, sp_search_num_playlists(result));
	else if(sp_search_total_playlists(result) < sp_search_num_playlists(result))
		test_report(userdata, "Total playlists (%d) less than number of playlists (%d)", sp_search_total_playlists(result), sp_search_num_playlists(result));
	else {
		if (!sp_search_playlist_name(result, 1)) {
			test_report(userdata, "Expected a name for playlist #1");
		} else
			test_ok(userdata);
	}
	sp_search_release(result);
}


static void SP_CALLCONV search2_cb(sp_search *result, void *userdata)
{
	if(!sp_search_is_loaded(result))
		test_report(userdata, "Result not loaded");
	else if(sp_search_error(result) != SP_ERROR_OK)
		test_report(userdata, "%s", sp_error_message(sp_search_error(result)));
	else
		test_ok(userdata);
	sp_search_release(result);
}

static void SP_CALLCONV did_you_mean_cb(sp_search *result, void *userdata)
{
	if(!sp_search_is_loaded(result))
		test_report(userdata, "Result not loaded");
	else if(sp_search_error(result) != SP_ERROR_OK)
		test_report(userdata, "%s", sp_error_message(sp_search_error(result)));
	else if(strcmp(sp_search_did_you_mean(result), "madonna"))
		test_report(userdata, "Expected '%s' but got '%s'",
			    "madonna", sp_search_did_you_mean(result));
	else
		test_ok(userdata);
	sp_search_release(result);
}


static void search_test(void)
{
	test_start(&search1);
	sp_search_create(g_session, "madonna", 1,2,3,4,5,6,7,8, SP_SEARCH_STANDARD, search1_cb, &search1);

	test_start(&search2);
	sp_search_create(g_session, "madonna", 0,250,0,250,0,250, 0, 0, SP_SEARCH_STANDARD, search2_cb, &search2);

	test_start(&did_you_mean);
	sp_search_create(g_session, "madonnnna", 0,250,0,250,0,250, 0, 0, SP_SEARCH_STANDARD, did_you_mean_cb, &did_you_mean);
}


/*****************************************************************
 * Album browse
 */
TEST_DECL(browse_50th_law, 3);

static void SP_CALLCONV album_cb(sp_albumbrowse *result, void *userdata)
{
	if(!sp_albumbrowse_is_loaded(result))
		test_report(userdata, "Result not loaded");
	else if(sp_albumbrowse_error(result) != SP_ERROR_OK)
		test_report(userdata, "%s", sp_error_message(sp_albumbrowse_error(result)));
	else if(sp_albumbrowse_num_tracks(result) != 11)
		test_report(userdata, "Expected 11 tracks but got %d", sp_albumbrowse_num_tracks(result));
	else if(strcmp("Introduction - Excerpt",
		       sp_track_name(sp_albumbrowse_track(result, 0))))
		test_report(userdata, "Expected track #1 to be %s but got %s",
			    "Introduction - Excerpt",
			    sp_track_name(sp_albumbrowse_track(result, 0)));
	else if(strcmp("Confront Your Mortality – the Sublime - Excerpt",
		       sp_track_name(sp_albumbrowse_track(result, 10))))
		test_report(userdata, "Expected track #11 to be %s but got %s",
			    "Confront Your Mortality – the Sublime - Excerpt",
			    sp_track_name(sp_albumbrowse_track(result, 10)));
	else
		test_ok_backend_duration(userdata, sp_albumbrowse_backend_request_duration(result));
	sp_albumbrowse_release(result);
}



static void albumbrowse_test(void)
{
	sp_album *alb;
	test_start(&browse_50th_law);

	alb = album_from_uri("spotify:album:1kSXIcMv2voPwcjaJiUoC5");
	sp_albumbrowse_create(g_session, alb, album_cb, &browse_50th_law);
	sp_album_release(alb);
}


/*****************************************************************
 * Artist browse no albums
 */
TEST_DECL(browse_david_guetta, 3);
TEST_DECL(browse_adele, 3);
TEST_DECL(browse_veronica_maggio, 3);
TEST_DECL(browse_rihanna, 3);
TEST_DECL(browse_elvis, 3);


static void SP_CALLCONV no_albums_cb(sp_artistbrowse *result, void *userdata)
{
	if(!sp_artistbrowse_is_loaded(result))
		test_report(userdata, "Result not loaded");
	else if(sp_artistbrowse_error(result) != SP_ERROR_OK)
		test_report(userdata, "%s", sp_error_message(sp_artistbrowse_error(result)));
	else if(sp_artistbrowse_num_tracks(result))
		test_report(userdata, "Expected 0 tracks but got %d", sp_artistbrowse_num_tracks(result));
	else if(sp_artistbrowse_num_albums(result))
		test_report(userdata, "Expected 0 albums but got %d", sp_artistbrowse_num_albums(result));
	else
		test_ok_backend_duration(userdata, sp_artistbrowse_backend_request_duration(result));
	sp_artistbrowse_release(result);
}



static void artistbrowse_test(void)
{
	sp_artist *art;
	test_start(&browse_david_guetta);
	test_start(&browse_adele);
	test_start(&browse_veronica_maggio);
	test_start(&browse_rihanna);
	test_start(&browse_elvis);

	art = artist_from_uri("spotify:artist:1Cs0zKBU1kc0i8ypK3B9ai");
	sp_artistbrowse_create(g_session, art, SP_ARTISTBROWSE_NO_ALBUMS, no_albums_cb, &browse_david_guetta);
	sp_artist_release(art);

	art = artist_from_uri("spotify:artist:4dpARuHxo51G3z768sgnrY");
	sp_artistbrowse_create(g_session, art, SP_ARTISTBROWSE_NO_ALBUMS, no_albums_cb, &browse_adele);
	sp_artist_release(art);

	art = artist_from_uri("spotify:artist:2OIWxN9xUhgUHkeUCWCaNs");
	sp_artistbrowse_create(g_session, art, SP_ARTISTBROWSE_NO_ALBUMS, no_albums_cb, &browse_veronica_maggio);
	sp_artist_release(art);

	art = artist_from_uri("spotify:artist:5pKCCKE2ajJHZ9KAiaK11H");
	sp_artistbrowse_create(g_session, art, SP_ARTISTBROWSE_NO_ALBUMS, no_albums_cb, &browse_rihanna);
	sp_artist_release(art);

	art = artist_from_uri("spotify:artist:43ZHCT0cAZBISjO8DG9PnE");
	sp_artistbrowse_create(g_session, art, SP_ARTISTBROWSE_NO_ALBUMS, no_albums_cb, &browse_elvis);
	sp_artist_release(art);
}


/*****************************************************************
 * Artist browse no tracks
 */
TEST_DECL(browse_no_tracks_elvis, 10);


static void SP_CALLCONV no_tracks_cb(sp_artistbrowse *result, void *userdata)
{
	if(!sp_artistbrowse_is_loaded(result))
		test_report(userdata, "Result not loaded");
	else if(sp_artistbrowse_error(result) != SP_ERROR_OK)
		test_report(userdata, "%s", sp_error_message(sp_artistbrowse_error(result)));
	else if(sp_artistbrowse_num_tracks(result))
		test_report(userdata, "Expected 0 tracks but got %d", sp_artistbrowse_num_tracks(result));
	else
		test_ok_backend_duration(userdata, sp_artistbrowse_backend_request_duration(result));
	sp_artistbrowse_release(result);
}



static void artistbrowse_no_tracks_test(void)
{
	sp_artist *art;
	test_start(&browse_no_tracks_elvis);

	art = artist_from_uri("spotify:artist:43ZHCT0cAZBISjO8DG9PnE");
	sp_artistbrowse_create(g_session, art, SP_ARTISTBROWSE_NO_TRACKS, no_tracks_cb, &browse_no_tracks_elvis);
	sp_artist_release(art);
}



/*****************************************************************
 * Toplist browse
 */
TEST_DECL5(browse_toplist_artist_global, 3,  0,   0,   100);
TEST_DECL5(browse_toplist_album_global,  3,  0,   100, 0);
TEST_DECL5(browse_toplist_track_global,  3,  100, 0,   0);
TEST_DECL5(browse_toplist_artist_user,   3,  0,   0,   20);
TEST_DECL5(browse_toplist_album_user,    3,  0,   20,  0);
TEST_DECL5(browse_toplist_track_user,    3,  20,  0,   0);
TEST_DECL5(browse_toplist_artist_SE,     3,  0,   0,   100);
TEST_DECL5(browse_toplist_album_SE,      3,  0,   100, 0);
TEST_DECL5(browse_toplist_track_SE,      3,  100, 0,   0);
TEST_DECL5(browse_toplist_artist_US,     3,  0,   0,   100);
TEST_DECL5(browse_toplist_album_US,      3,  0,   100, 0);
TEST_DECL5(browse_toplist_track_US,      3,  100, 0,   0);

static void SP_CALLCONV toplist_cb(sp_toplistbrowse *result, void *userdata)
{
	const Test *t = userdata;
	if(!sp_toplistbrowse_is_loaded(result))
		test_report(userdata, "Result not loaded");
	else if(sp_toplistbrowse_error(result) != SP_ERROR_OK)
		test_report(userdata, "%s", sp_error_message(sp_toplistbrowse_error(result)));
	else if(t->arg1 != sp_toplistbrowse_num_tracks(result))
		test_report(userdata, "Expected %d tracks but got %d",
			    t->arg1, sp_toplistbrowse_num_tracks(result));
	else if(t->arg2 != sp_toplistbrowse_num_albums(result))
		test_report(userdata, "Expected %d albums but got %d",
			    t->arg2, sp_toplistbrowse_num_albums(result));
	else if(t->arg3 != sp_toplistbrowse_num_artists(result))
		test_report(userdata, "Expected %d artists but got %d",
			    t->arg3, sp_toplistbrowse_num_artists(result));
	else
		test_ok_backend_duration(userdata, sp_toplistbrowse_backend_request_duration(result));
	sp_toplistbrowse_release(result);
}



static void toplistbrowse_test(void)
{
	test_start(&browse_toplist_artist_global);
	test_start(&browse_toplist_album_global);
	test_start(&browse_toplist_track_global);

	test_start(&browse_toplist_artist_user);
	test_start(&browse_toplist_album_user);
	test_start(&browse_toplist_track_user);

	test_start(&browse_toplist_artist_SE);
	test_start(&browse_toplist_album_SE);
	test_start(&browse_toplist_track_SE);

	test_start(&browse_toplist_artist_US);
	test_start(&browse_toplist_album_US);
	test_start(&browse_toplist_track_US);

	sp_toplistbrowse_create(g_session, SP_TOPLIST_TYPE_ARTISTS,
				SP_TOPLIST_REGION_EVERYWHERE, NULL,
				toplist_cb, &browse_toplist_artist_global);

	sp_toplistbrowse_create(g_session, SP_TOPLIST_TYPE_ALBUMS,
				SP_TOPLIST_REGION_EVERYWHERE, NULL,
				toplist_cb, &browse_toplist_album_global);

	sp_toplistbrowse_create(g_session, SP_TOPLIST_TYPE_TRACKS,
				SP_TOPLIST_REGION_EVERYWHERE, NULL,
				toplist_cb, &browse_toplist_track_global);


	sp_toplistbrowse_create(g_session, SP_TOPLIST_TYPE_ARTISTS,
				SP_TOPLIST_REGION_USER, NULL,
				toplist_cb, &browse_toplist_artist_user);

	sp_toplistbrowse_create(g_session, SP_TOPLIST_TYPE_ALBUMS,
				SP_TOPLIST_REGION_USER, NULL,
				toplist_cb, &browse_toplist_album_user);

	sp_toplistbrowse_create(g_session, SP_TOPLIST_TYPE_TRACKS,
				SP_TOPLIST_REGION_USER, NULL,
				toplist_cb, &browse_toplist_track_user);



	sp_toplistbrowse_create(g_session, SP_TOPLIST_TYPE_ARTISTS,
				SP_TOPLIST_REGION('S', 'E'), NULL,
				toplist_cb, &browse_toplist_artist_SE);

	sp_toplistbrowse_create(g_session, SP_TOPLIST_TYPE_ALBUMS,
				SP_TOPLIST_REGION('S', 'E'), NULL,
				toplist_cb, &browse_toplist_album_SE);

	sp_toplistbrowse_create(g_session, SP_TOPLIST_TYPE_TRACKS,
				SP_TOPLIST_REGION('S', 'E'), NULL,
				toplist_cb, &browse_toplist_track_SE);


	sp_toplistbrowse_create(g_session, SP_TOPLIST_TYPE_ARTISTS,
				SP_TOPLIST_REGION('U', 'S'), NULL,
				toplist_cb, &browse_toplist_artist_US);

	sp_toplistbrowse_create(g_session, SP_TOPLIST_TYPE_ALBUMS,
				SP_TOPLIST_REGION('U', 'S'), NULL,
				toplist_cb, &browse_toplist_album_US);

	sp_toplistbrowse_create(g_session, SP_TOPLIST_TYPE_TRACKS,
				SP_TOPLIST_REGION('U', 'S'), NULL,
				toplist_cb, &browse_toplist_track_US);
}

/*****************************************************************
 * Radio
 */
#ifdef SP_WITH_RADIO

TEST_DECL(radio_artist,3);
TEST_DECL(radio_track,3);
TEST_DECL(radio_genre,3);


static void radio_cb(sp_radio *result, void *userdata)
{
	int num_tracks;
	sp_track* track;

	if(!sp_radio_is_loaded(result))
		test_report(userdata, "Result not loaded");
	else if(sp_radio_error(result) != SP_ERROR_OK)
		test_report(userdata, "%s", sp_error_message(sp_radio_error(result)));
	else {
		num_tracks=sp_radio_num_tracks(result);
		if (num_tracks>0) {
			track = sp_radio_track(result, num_tracks - 1);
			if ( track ) {
				test_ok(userdata);
			} else {
				test_report(userdata, "Can't get last track");
			}
		} else {
			test_report(userdata, "No tracks returned");
		}
	}
	sp_radio_release(result);
}

static void radio_test(void)
{
	sp_link* link;
	test_start(&radio_artist);
	link = sp_link_create_from_string("spotify:artist:6tbjWDEIzxoDsBA1FuhfPW"); // Madonna
	sp_radio_create_from_link(g_session, link, &radio_cb, &radio_artist);
	sp_link_release( link );

	test_start(&radio_track);
	link = sp_link_create_from_string("spotify:track:1z3ugFmUKoCzGsI6jdY4Ci"); // Like A Prayer
	sp_radio_create_from_link(g_session, link, &radio_cb, &radio_track);
	sp_link_release( link );

	test_start(&radio_genre);
	sp_radio_create_from_genre(g_session, SP_RADIO_GENRE_DANCE, &radio_cb, &radio_genre);
}

#endif
/*****************************************************************
 * Streaming test
 */

TEST_DECL(playtrack, 25);

static sp_track *stream_track;
static int stream_track_end;


/**
 * Callback from spotify session. Happen on different thread so we need to signal and wakeup
 */
void SP_CALLCONV end_of_track(sp_session *s)
{
	stream_track_end = 1;
	notify_main_thread(g_session);
	info_report("end of track");
}

/**
 * Callback from spotify session. Just consume all frames
 */
int SP_CALLCONV music_delivery(sp_session *s, const sp_audioformat *fmt, const void *frames, int num_frames)
{
	return num_frames;
}

/**
 * Callback from spotify session. Happen on different thread so we need to signal and wakeup
 */
void SP_CALLCONV play_token_lost(sp_session *s)
{
	stream_track_end = 2;
	notify_main_thread(g_session);
	info_report("Playtoken lost");
}


static void playtrack_test(void)
{
	sp_error err;
	test_start(&playtrack);
	if((err = sp_session_player_load(g_session, stream_track)) != SP_ERROR_OK) {
		test_report(&playtrack, "Unable to load track: %s",  sp_error_message(err));
		return;
	}
	info_report("Streaming '%s' by '%s' this will take a while", sp_track_name(stream_track),
		    sp_artist_name(sp_track_artist(stream_track, 0)));
	sp_session_player_play(g_session, 1);
}

static int check_streaming_done(void)
{
	if(stream_track_end == 2)
		test_report(&playtrack, "Playtoken lost");
	else if(stream_track_end == 1)
		test_ok(&playtrack);
	else
		return 0;
	stream_track_end = 0;
	return 1;
}

/*****************************************************************
 * Image loading
 */
TEST_DECL3(image1, 3, 4560901);
TEST_DECL3(image2, 3, 4920563);
TEST_DECL3(image3, 3, 4440836);
TEST_DECL3(image4, 3, 2202765);
TEST_DECL3(image5, 3, 3254083);
TEST_DECL3(image6, 3, 2595372);
TEST_DECL3(image7, 3, 10028876);
TEST_DECL3(image8, 3, 7017370);
TEST_DECL3(image9, 3, 3655831);

static void SP_CALLCONV image_cb(sp_image *image, void *userdata)
{
	Test *t = userdata;
	size_t size;
	const byte *data;
	int sum;
	size_t i;

	data = sp_image_data(image, &size);
	sum = size;
	for(i = 0; i < size; i++)
		sum += data[i];

	sp_image_release(image);
	if(sum != t->arg1)
		test_report(userdata, "Invalid checksum");
	else
		test_ok(userdata);
}


static void load_image(Test *t, const char *uri)
{
	sp_link *l;
	sp_image *img;

	test_start(t);

	l = sp_link_create_from_string(uri);
	img = sp_image_create_from_link(g_session, l);
	sp_image_add_load_callback(img, image_cb, t);
	sp_link_release(l);

}

static void image_test(void)
{
	load_image(&image1, "spotify:image:34062b6f0168af33b9cd80c630a38f5de183f936");
	load_image(&image2, "spotify:image:654873a4d6a648f24055a5faec324aa2a80a997d");
	load_image(&image3, "spotify:image:5a9b36ac716b1178dc35bd05db96a34eafb77ad4");
	load_image(&image4, "spotify:image:564ce92718a792307f40634cb3225f0e91d7965b");
	load_image(&image5, "spotify:image:67b169a8a8df439437df7424ccd5dd7b7d88fca8");
	load_image(&image6, "spotify:image:b05e1aaf5824c9e4736b5d7b173aec5fdd4e6a4e");
	load_image(&image7, "spotify:image:0f44e39b1e7c21701640bb5ad035efde02b6aeeb");
	load_image(&image8, "spotify:image:c588570590c7d76670efad294424b991ddde17a0");
	load_image(&image9, "spotify:image:53a580c6e3e314838dcbc2ed97ebebb54a25693e");
}




/*********************************************************************************
 * State machine
 */
#define WAIT_FOR_TEST(t) state = __LINE__; case __LINE__: if(!(t)->done) return
#define WAIT_FOR(expr)   state = __LINE__; case __LINE__: if(!(expr)) return

static void print_resource_usage()
{
#if __APPLE__ || __linux__
	struct rusage r_usage;
	int res;

	res = getrusage(RUSAGE_SELF, &r_usage);
	if (res == 0) {
		printf("getrusage() returned:\n");

#if __APPLE__
#define TV_USEC_FORMAT "d"
#elif __linux__
#define TV_USEC_FORMAT "ld"
#endif
		printf("  User time:   %ld.%03" TV_USEC_FORMAT "s\n", r_usage.ru_utime.tv_sec, r_usage.ru_utime.tv_usec / 1000);
		printf("  System time: %ld.%03" TV_USEC_FORMAT "s\n", r_usage.ru_stime.tv_sec, r_usage.ru_stime.tv_usec / 1000);
#undef TV_USEC_FORMAT

#if __APPLE__
#define MAXRSS_UNITS "bytes"
#elif __linux__
#define MAXRSS_UNITS "kilobytes"
#endif
		printf("  Peak memory usage: %ld " MAXRSS_UNITS " (ru_maxrss)\n", r_usage.ru_maxrss); // integral max resident set size
#undef MAXRSS_UNITS

	} else {
		perror("getrusage() failed");
	}
#endif // __APPLE__ || __linux__
}

/**
 *
 */
void test_process(void)
{
	static int state;
	switch(state) {
	case 0:
		search_test();
		WAIT_FOR(!active_tests);

		albumbrowse_test();
		WAIT_FOR(!active_tests);

		artistbrowse_test();
		WAIT_FOR(!active_tests);

		artistbrowse_no_tracks_test();
		WAIT_FOR(!active_tests);

		image_test();
		WAIT_FOR(!active_tests);

		toplistbrowse_test();
		WAIT_FOR(!active_tests);

#ifdef SP_WITH_RADIO
		radio_test();
		WAIT_FOR(!active_tests);
#endif

		info_report("Loading %s", "spotify:track:5iIeIeH3LBSMK92cMIXrVD");
		stream_track = track_from_uri("spotify:track:5iIeIeH3LBSMK92cMIXrVD");
		WAIT_FOR(sp_track_is_loaded(stream_track));

		playtrack_test();

		WAIT_FOR(check_streaming_done());
		WAIT_FOR(!active_tests);
		sp_track_release(stream_track);

		state = -1;
		test_finished();
		print_resource_usage();
	}
}


int cmd_test(int argc, char **argv)
{
	extern int g_selftest;
	g_selftest = 1;
	test_process();
	return -1;
}
