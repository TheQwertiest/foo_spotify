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

#ifndef CMD_H__
#define CMD_H__

extern void cmd_exec_unparsed(char *l);

extern void cmd_dispatch(int argc, char **argv);

extern void cmd_done(void);


extern int cmd_log(int argc, char **argv);
extern int cmd_logout(int argc, char **argv);
extern int cmd_browse(int argc, char **argv);
extern int cmd_search(int argc, char **argv);
extern int cmd_whatsnew(int argc, char **argv);
extern int cmd_toplist(int argc, char **argv);
extern int cmd_post(int argc, char **argv);
extern int cmd_star(int argc, char **argv);
extern int cmd_unstar(int argc, char **argv);
extern int cmd_starred(int argc, char **argv);
extern int cmd_inbox(int argc, char **argv);
extern int cmd_friends(int argc, char **argv);
extern int cmd_facebook_scrobbling(int argc, char **argv);
extern int cmd_is_facebook_scrobbling(int argc, char **argv);
extern int cmd_spotify_social(int argc, char **argv);
extern int cmd_is_spotify_social(int argc, char **argv);
extern int cmd_set_lastfm_scrobbling_credentials(int argc, char** argv);
extern int cmd_lastfm_scrobbling(int argc, char** argv);
extern int cmd_is_lastfm_scrobbling(int argc, char** argv);
extern int cmd_private_session(int argc, char** argv);
extern int cmd_is_private_session(int argc, char** argv);

extern int cmd_playlists(int argc, char **argv);
extern int cmd_playlist(int argc, char **argv);
extern int cmd_set_autolink(int argc, char **argv);

extern int cmd_published_playlists(int argc, char **argv);
extern int cmd_get_published_playlist (int argc, char **argv);

extern int cmd_social_enable(int argc, char **argv);
extern int cmd_playlists_enable(int argc, char **argv);
extern int cmd_add_folder(int argc, char **argv);
extern int cmd_update_subscriptions(int argc, char **argv);
extern int cmd_playlist_add_track(int argc, char **argv);
extern int cmd_playlist_offline(int argc, char **argv);

#if WITH_TEST_COMMAND
extern int cmd_test(int argc, char **argv);
#endif

/* Shared functions */
void browse_playlist(sp_playlist *pl);
void print_track(sp_track *track);

#endif // CMD_H__
