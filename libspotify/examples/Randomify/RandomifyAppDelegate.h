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
 *
 * This example application show how to use libspotify in a Mac OS X
 * application, playing randomly from your Starred playlist.
 *
 * This file is part of the libspotify examples suite.
 */

#import <Cocoa/Cocoa.h>
#import "SpotifySession.h"

@interface RandomifyAppDelegate : NSObject
	<SpotifySessionDelegate, SpotifyPlaylistDelegate>
{
    IBOutlet NSWindow *window;
	IBOutlet NSTextField *username;
	IBOutlet NSTextField *password;
	IBOutlet NSTextField *log;
	IBOutlet NSTextField *status;
	
	SpotifySession *session;
	SpotifyPlaylist *starred;
	BOOL playing;
	IBOutlet NSButton *playButton;
	
	SpotifyTrack *playingTrack;
}
-(IBAction)loginAndPlay:(id)sender;

-(IBAction)playARandomSong:(id)sender;
-(IBAction)togglePlay:(id)sender;
@end
