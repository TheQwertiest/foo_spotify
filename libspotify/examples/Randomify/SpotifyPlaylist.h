/**
 * Copyright (c) 2006-2010 Spotify Ltd
 * This file is part of the libspotify examples suite.
 * See RandomifyAppDelegate.h for license.
 */
 
#import <Foundation/Foundation.h>
#import <libspotify/api.h>
#import "SpotifyTrack.h"

@class SpotifyPlaylist;
@protocol SpotifyPlaylistDelegate
@optional
-(void)playlistBeganUpdating:(SpotifyPlaylist*)playlist;
-(void)playlistEndedUpdating:(SpotifyPlaylist*)playlist;
-(void)playlistChangedState:(SpotifyPlaylist*)playlist;
@end


@interface SpotifyPlaylist : NSObject {
	sp_playlist *playlist;
	sp_playlist_callbacks callbacks;
	sp_session *session;
	NSObject<SpotifyPlaylistDelegate> *delegate;
}
-(id)initWithPlaylist:(sp_playlist*)playlist_ onSession:(sp_session*)sess;
@property (assign) NSObject<SpotifyPlaylistDelegate> *delegate;
@property (readonly) BOOL loaded; // listen to playlistEndedUpdating if false

-(NSUInteger)countOfTracks;
-(SpotifyTrack*)objectInTracksAtIndex:(NSUInteger)index;
@end
