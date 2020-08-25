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

#include "spshell.h"
#include "cmd.h"

static HANDLE events[2];

static int enable_console;
static char console_buf[1024];
static int console_ptr;

extern int is_logged_out;

extern int g_selftest;

/**
 * Very simplistic console input
 */
static void console_input(void)
{
	TCHAR inp;
	DWORD read;
	INPUT_RECORD rec;

	if(!ReadConsoleInput(events[1], &rec, 1, &read))
		return;

	if(read == 0)
		return;

	if(rec.EventType != KEY_EVENT)
		return;

	if(!rec.Event.KeyEvent.bKeyDown)
		return;

	inp = rec.Event.KeyEvent.uChar.AsciiChar;


	switch(inp) {
	case 8:
		if(console_ptr > 0) {
			console_ptr--;
			printf("\b \b");
		}
		break;
	case 13:
		printf("\n\r");
		console_buf[console_ptr++] = 0;
		console_ptr = 0;
		enable_console = 0;
		cmd_exec_unparsed(console_buf);
		break;

	default:
		if(console_ptr < sizeof(console_buf) - 1) {
			printf("%c", (char)inp);
			console_buf[console_ptr++] = inp;
		}
		break;
	}
}

static void trim(char *buf)
{
	size_t l = strlen(buf);
	while(l > 0 && buf[l - 1] < 32)
		buf[--l] = 0;
}


int __cdecl main(int argc, char **argv)
{
	const char *username = argc > 1 ? argv[1] : NULL;
	const char *blob = argc > 2 ? argv[2] : NULL;
	const char *password = NULL;
	int selftest = argc > 3 ? !strcmp(argv[3], "selftest") : 0;
	char username_buf[256];
	char password_buf[256];
	int r;
	int next_timeout = 0;
	DWORD ev;
	DWORD mode;

	events[0] = CreateEvent(NULL, FALSE, FALSE, NULL);
	events[1] = GetStdHandle(STD_INPUT_HANDLE);

	printf("Using libspotify %s\n", sp_build_id());

	if (username == NULL) {
		printf("Username: (just press enter to login with stored credentials): ");
		fflush(stdout);
		fgets(username_buf, sizeof(username_buf), stdin);
		trim(username_buf);
		if(username_buf[0] == 0) {
			username = NULL;
		} else {
			username = username_buf;
		}
	}

	// If a username was supplied but no blob, prompt for password
	if (username != NULL && blob == NULL) {
		printf("Password: ");
		fflush(stdout);

		if (!GetConsoleMode(events[1], &mode) ||
			!SetConsoleMode(events[1], mode & ~ENABLE_ECHO_INPUT)) {
			printf("Unable to set console mode err=%d\n", GetLastError());
			exit(1);
		}

		fgets(password_buf, sizeof(password_buf), stdin);
		trim(password_buf);
		password = password_buf;
		printf("\r\n");
	}

	if ((r = spshell_init(username, password, blob, selftest)) != 0)
		exit(r);

	if (!SetConsoleMode(events[1], ENABLE_PROCESSED_INPUT)) {
		printf("Unable to set console mode err=%d\n", GetLastError());
		exit(1);
	}

	while(!is_logged_out) {
		ev = WaitForMultipleObjects(1 + enable_console, events, FALSE, next_timeout > 0 ? next_timeout : INFINITE);
		switch (ev) {
		case WAIT_OBJECT_0 + 0:
		case WAIT_TIMEOUT:
			do {
				sp_session_process_events(g_session, &next_timeout);
			} while (next_timeout == 0);

			if(g_selftest)
				test_process();
			break;

		case WAIT_OBJECT_0 + 1:
			console_input();
			break;
		}
	}
	printf("Logged out\n");
	sp_session_release(g_session);
	printf("Exiting...\n");
	return 0;
}

/**
 *
 */
void cmd_done(void)
{
	enable_console = 1;
	printf("> ");
	fflush(stdout);
}


/**
 *
 */
void start_prompt(void)
{
	cmd_done();
}


/**
 *
 */
void SP_CALLCONV notify_main_thread(sp_session *session)
{
	SetEvent(events[0]);
}

/**
 *
 */
sp_uint64 get_ts(void)
{
	return GetTickCount();
}
