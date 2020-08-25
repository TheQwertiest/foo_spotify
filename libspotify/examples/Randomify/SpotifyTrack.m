/**
 * Copyright (c) 2006-2010 Spotify Ltd
 * This file is part of the libspotify examples suite.
 * See RandomifyAppDelegate.h for license.
 */

#import "SpotifyTrack.h"


@implementation SpotifyTrack
@synthesize track;
-(id)initWithTrack:(sp_track*)track_ onSession:(sp_session*)sess;
{
	track = track_;
	session = sess;
	sp_track_add_ref(track);
	return self;
}
-(void)dealloc;
{
	sp_track_release(track);
	[super dealloc];
}

-(NSString*)name;
{
	const char *name = sp_track_name(track);
	if(!name || !sp_track_is_loaded(track)) return @"[loading]";
	return [NSString stringWithUTF8String:sp_track_name(track)];
}
-(NSString*)artistName;
{
	sp_artist *artist = sp_track_artist(track, 0);
	const char *artistName = artist ? sp_artist_name(artist) : "[loading]";
	return [NSString stringWithUTF8String:artistName];
}
-(BOOL)loaded;
{
	return sp_track_is_loaded(track);
}
-(BOOL)available;
{
	return sp_track_get_availability(session, track) == SP_TRACK_AVAILABILITY_AVAILABLE;
}

-(NSString*)description;
{
	return [NSString stringWithFormat:@"<%@ by %@>",
		self.name,
		self.artistName
	];
}

@end
