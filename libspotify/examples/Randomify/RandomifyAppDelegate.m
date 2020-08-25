/**
 * Copyright (c) 2006-2010 Spotify Ltd
 * This file is part of the libspotify examples suite.
 * See RandomifyAppDelegate.h for license.
 */

#import "RandomifyAppDelegate.h"
#import "SpotifySession.h"

@interface RandomifyAppDelegate ()
@property (retain) SpotifyPlaylist *starred;
@property (retain) SpotifyTrack *playingTrack;
@property BOOL playing;
@end


@implementation RandomifyAppDelegate
@synthesize starred, playingTrack;
#pragma mark Session delegates
-(void)session:(SpotifySession*)session_ logged:(NSString*)logmsg;
{
	NSString *new = [logmsg stringByAppendingString:log.stringValue];
	log.stringValue = new;
}
-(void)sessionLoggedIn:(SpotifySession*)session_ error:(NSError*)err;
{
	if(err) {
		[NSApp presentError:err];
		return;
	}
	[self session:session logged:@"Logged in!\n"];
	
	self.starred = session.starred;
	starred.delegate = self;

	[self playARandomSong:nil];
}
-(void)session:(SpotifySession*)session_ connectionError:(NSError*)error;
{
	[NSApp presentError:error];
}
-(void)session:(SpotifySession*)session_ streamingError:(NSError*)error;
{
	[NSApp presentError:error];
}

-(void)playlistEndedUpdating:(SpotifyPlaylist*)playlist;
{
	if(playlist.loaded && !playing)
		[self playARandomSong:nil];
}
-(void)playlistChangedState:(SpotifyPlaylist*)playlist;
{
	if(playlist.loaded && !playing)
		[self playARandomSong:nil];
}

-(void)sessionUpdatedMetadata:(SpotifySession *)session_;
{
	// A track we're waiting for metadata for might now have gotten it.
	if(!playing && playingTrack && playingTrack.loaded) {
		if(!playingTrack.available) {
			[self playARandomSong:nil];
			return;
		}
		[self session:session logged:[NSString stringWithFormat:@"Playing now loaded track: %@\n", playingTrack]];
		status.stringValue = playingTrack.description;
		[session loadTrack:playingTrack];
		[session play];
		self.playing = YES;
	}
}
-(void)sessionEndedPlayingTrack:(SpotifySession*)session_;
{
	self.playing = NO;
	[self session:session logged:@"Playlist is ready!\n"];
	[self playARandomSong:nil];
}
-(void)sessionLostPlayToken:(SpotifySession *)session;
{
	NSRunAlertPanel(@"Lost play token", @"Another client is playing.", @"Arrrghh", nil, nil);
	self.playing = NO;
}


#pragma mark -
- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
	srandom(time(NULL));
	self.playing = NO;

	NSError *err = nil;
	session = [[SpotifySession alloc] initError:&err];
	session.delegate = self;
	
	status.stringValue = @"Randomify";
	
	if(!session) {
		[NSApp presentError:err];
		[NSApp terminate:nil];
	}
}
-(IBAction)loginAndPlay:(id)sender;
{
	[session loginUser:username.stringValue password:password.stringValue];
}

-(IBAction)playARandomSong:(id)sender;
{
	NSUInteger count = [starred countOfTracks];

	if(!starred.loaded || !count) {
		[self session:session logged:@"Waiting for playlist to load...\n"];
		return;
	}
	
	NSUInteger randomIndex = random()%count;
	
	self.playingTrack = [starred objectInTracksAtIndex:randomIndex];
	if(!self.playingTrack.loaded) {
		[self session:session logged:@"Waiting for track to load...\n"];
		status.stringValue = @"Loading…";
		return;
	}
	
	[self session:session logged:[NSString stringWithFormat:@"Playing track: %@\n", playingTrack]];
	status.stringValue = playingTrack.description;
	
	[session loadTrack:playingTrack];
	[session play];

	self.playing = YES;
}
-(IBAction)togglePlay:(id)sender;
{
	if(playing) {
		[session pause];
		self.playing = NO;
	} else {
		[session play];
		self.playing = YES;
	}
}

-(BOOL)playing;
{
	return playing;
}
-(void)setPlaying:(BOOL)playing_;
{
	[playButton setEnabled:playingTrack != nil];
	
	playing = playing_;
	[playButton setStringValue:playing ? @"║" : @"▹"];
}
@end
