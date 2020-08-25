/**
 * Copyright (c) 2006-2010 Spotify Ltd
 * This file is part of the libspotify examples suite.
 * See RandomifyAppDelegate.h for license.
 */

#import <Foundation/Foundation.h>
#import <libspotify/api.h>
#import "audio.h"
#import "SpotifyPlaylist.h"

@class SpotifySession;
@protocol SpotifySessionDelegate
@optional
-(void)sessionLoggedIn:(SpotifySession*)session error:(NSError*)err;
-(void)sessionLoggedOut:(SpotifySession*)session;
-(void)sessionUpdatedMetadata:(SpotifySession*)session;
-(void)session:(SpotifySession*)session connectionError:(NSError*)error;
-(void)session:(SpotifySession*)session hasMessageToUser:(NSString*)message;
-(void)sessionLostPlayToken:(SpotifySession*)session;
-(void)session:(SpotifySession*)session logged:(NSString*)logmsg;
-(void)sessionEndedPlayingTrack:(SpotifySession*)session;
-(void)session:(SpotifySession*)session streamingError:(NSError*)error;
-(void)sessionUpdatedUserinfo:(SpotifySession*)session;
@end


@interface SpotifySession : NSObject {
	sp_session_config config;
	sp_session_callbacks callbacks;
	NSString *cachesDir, *supportDir;
	audio_fifo_t audiofifo;
	sp_session *session;
	NSObject<SpotifySessionDelegate> *delegate;
}
@property (assign) NSObject<SpotifySessionDelegate> *delegate;
-(id)initError:(NSError**)err;
-(void)loginUser:(NSString*)user password:(NSString*)passwd;
-(void)logout;

@property (readonly) SpotifyPlaylist *starred;

// Should be its own class: SpotifyPlayer
-(void)loadTrack:(SpotifyTrack*)track;
-(void)pause;
-(void)play;


// Stops processing events. If you only release this object,
// the runtime will still own it.
-(void)shutdown;
@end
