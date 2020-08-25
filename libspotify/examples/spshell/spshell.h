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

#ifndef SPSHELL_H__
#define SPSHELL_H__

#include <stdlib.h>
#ifndef WIN32
#include <stdint.h>
#endif
#include <stdio.h>
#include <libspotify/api.h>

#define WITH_TEST_COMMAND 1

#if WIN32
#include <windows.h>
#define snprintf sprintf_s
#define strcasecmp lstrcmp
#endif 

#ifndef PRIx64
#define PRIx64 "llx"
#endif

#if USE_STRCMP
#define strcasecmp strcmp
#endif

extern sp_session *g_session;

extern void (*metadata_updated_fn)(void);

extern int spshell_init(const char *username, const char *password, const char *blob, int selftest);

extern void SP_CALLCONV notify_main_thread(sp_session *session);

extern void start_prompt(void);

extern sp_uint64 get_ts(void);

#if WITH_TEST_COMMAND

extern void test_finished(void);

extern void test_process(void);

extern void SP_CALLCONV end_of_track(sp_session *s);

extern int SP_CALLCONV music_delivery(sp_session *s, const sp_audioformat *fmt, const void *frames, int num_frames);

extern void SP_CALLCONV play_token_lost(sp_session *s);

#endif

#endif // SPSHELL_H__
