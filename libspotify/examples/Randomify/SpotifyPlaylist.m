/**
 * Copyright (c) 2006-2010 Spotify Ltd
 * This file is part of the libspotify examples suite.
 * See RandomifyAppDelegate.h for license.
 */

#import "SpotifyPlaylist.h"
#import "SpotifyTrack.h"

#define plobj ((SpotifyPlaylist*)userdata)

static void playlist_update_in_progress(sp_playlist *pl, bool done, void *userdata)
{
	if(!done) {
		if([plobj.delegate respondsToSelector:@selector(playlistBeganUpdating:)])
			[plobj.delegate playlistBeganUpdating:plobj];
	} else {
		if([plobj.delegate respondsToSelector:@selector(playlistEndedUpdating:)])
			[plobj.delegate playlistEndedUpdating:plobj];
	}
}
static void playlist_state_changed(sp_playlist *pl, void *userdata)
{
	if([plobj.delegate respondsToSelector:@selector(playlistChangedState:)])
		[plobj.delegate playlistChangedState:plobj];	
}

// TODO: Instead of exposing playlist content changes as delegate methods,
// make tracks KVO-compliant

@implementation SpotifyPlaylist
@synthesize delegate;
-(id)initWithPlaylist:(sp_playlist*)playlist_ onSession:(sp_session*)sess;
{
	playlist = playlist_;
	
	session = sess;
	
	callbacks = (sp_playlist_callbacks) {
		.playlist_update_in_progress = playlist_update_in_progress,
	};
	
	sp_playlist_add_ref(playlist);
	sp_playlist_add_callbacks(playlist, &callbacks, self);
	
	
	return self;
}
-(void)dealloc;
{
	sp_playlist_remove_callbacks(playlist, &callbacks, self);
	sp_playlist_release(playlist);
	[super dealloc];
}

-(BOOL)loaded;
{
	return sp_playlist_is_loaded(playlist);
}
-(NSUInteger)countOfTracks;
{
	return sp_playlist_num_tracks(playlist);
}
-(SpotifyTrack*)objectInTracksAtIndex:(NSUInteger)index;
{
	return [[[SpotifyTrack alloc] initWithTrack:sp_playlist_track(playlist, index) onSession:session] autorelease];
}
@end
