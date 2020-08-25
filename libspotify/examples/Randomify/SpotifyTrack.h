/**
 * Copyright (c) 2006-2010 Spotify Ltd
 * This file is part of the libspotify examples suite.
 * See RandomifyAppDelegate.h for license.
 */

#import <Foundation/Foundation.h>
#import <libspotify/api.h>

@interface SpotifyTrack : NSObject {
	sp_track *track;
	sp_session *session;
}
-(id)initWithTrack:(sp_track*)track_ onSession:(sp_session*)sess;

@property (readonly) NSString *name;
@property (readonly) NSString *artistName;
@property (readonly) BOOL loaded;
@property (readonly) BOOL available;

// Private
@property (readonly) sp_track *track;
@end
