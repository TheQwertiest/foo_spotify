/**
 * Copyright (c) 2006-2010 Spotify Ltd
 * This file is part of the libspotify examples suite.
 * See RandomifyAppDelegate.h for license.
 */

#import "SpotifySession.h"

/// The application key is specific to each project, and allows Spotify
/// to produce statistics on how our service is used.
extern const uint8_t g_appkey[];
/// The size of the application key.
extern const size_t g_appkey_size;

NSError *makeError(sp_error code) {
	NSError *err = [NSError errorWithDomain:@"com.spotify" code:code userInfo:[NSDictionary dictionaryWithObjectsAndKeys:
		[NSString stringWithUTF8String:sp_error_message(code)], NSLocalizedDescriptionKey,
		nil
	]];
	return err;
}

@interface SpotifySession ()
-(void)processEvents;
@property (readonly) audio_fifo_t *audiofifo;

-(void)tellDelegateThatSessionLogged:(NSString*)msg;

@end

#pragma mark 
#pragma mark Callbacks
#pragma mark -

static SpotifySession *sessobj(sp_session *session) {
	return (SpotifySession*)sp_session_userdata(session);
}

static void logged_in(sp_session *session, sp_error error)
{
	NSObject<SpotifySessionDelegate> *delegate = sessobj(session).delegate;
	NSError *err = SP_ERROR_OK == error ? nil : makeError(error);
	if([delegate respondsToSelector:@selector(sessionLoggedIn:error:)])
		[delegate sessionLoggedIn:sessobj(session) error:err];
}

static void logged_out(sp_session *session)
{
	NSObject<SpotifySessionDelegate> *delegate = sessobj(session).delegate;
	if([delegate respondsToSelector:@selector(sessionLoggedOut:)])
		[delegate sessionLoggedOut:sessobj(session)];
}

static void metadata_updated(sp_session *session)
{
	NSObject<SpotifySessionDelegate> *delegate = sessobj(session).delegate;
	if([delegate respondsToSelector:@selector(sessionUpdatedMetadata:)])
		[delegate sessionUpdatedMetadata:sessobj(session)];
}

static void connection_error(sp_session *session, sp_error error)
{
	NSObject<SpotifySessionDelegate> *delegate = sessobj(session).delegate;
	NSError *err = SP_ERROR_OK == error ? nil : makeError(error);
	if([delegate respondsToSelector:@selector(session:connectionError:)])
		[delegate session:sessobj(session) connectionError:err];
}

static void message_to_user(sp_session *session, const char *message)
{
	NSObject<SpotifySessionDelegate> *delegate = sessobj(session).delegate;
	NSString *msg = [NSString stringWithUTF8String:message];
	if([delegate respondsToSelector:@selector(session:hasMessageToUser:)])
		[delegate session:sessobj(session) hasMessageToUser:msg];
}

static void notify_main_thread(sp_session *session)
{
	[sessobj(session) performSelectorOnMainThread:@selector(processEvents) withObject:nil waitUntilDone:NO];
}

static int music_delivery(sp_session *sess, const sp_audioformat *format,
                          const void *frames, int num_frames)
{
	audio_fifo_t *af = sessobj(sess).audiofifo;
	audio_fifo_data_t *afd = NULL;
	size_t s;

	if (num_frames == 0)
		return 0; // Audio discontinuity, do nothing

	pthread_mutex_lock(&af->mutex);

	/* Buffer one second of audio */
	if (af->qlen > format->sample_rate) {
		pthread_mutex_unlock(&af->mutex);

		return 0;
	}

	s = num_frames * sizeof(int16_t) * format->channels;

	afd = malloc(sizeof(audio_fifo_data_t) + s);
	memcpy(afd->samples, frames, s);

	afd->nsamples = num_frames;

	afd->rate = format->sample_rate;
	afd->channels = format->channels;

	TAILQ_INSERT_TAIL(&af->q, afd, link);
	af->qlen += num_frames;

	pthread_cond_signal(&af->cond);
	pthread_mutex_unlock(&af->mutex);

	return num_frames;
}

static void play_token_lost(sp_session *session)
{
	NSObject<SpotifySessionDelegate> *delegate = sessobj(session).delegate;
	if([delegate respondsToSelector:@selector(sessionLostPlayToken:)])
		[delegate sessionLostPlayToken:sessobj(session)];
}

static void log_message(sp_session *session, const char *message)
{
	NSObject<SpotifySessionDelegate> *delegate = sessobj(session).delegate;
	NSString *msg = [[NSString alloc] initWithUTF8String:message];
	if([delegate respondsToSelector:@selector(session:logged:)])
		[sessobj(session) performSelectorOnMainThread:@selector(tellDelegateThatSessionLogged:)
										   withObject:msg 
										waitUntilDone:NO];
	
}

static void end_of_track(sp_session *session)
{
	NSObject<SpotifySessionDelegate> *delegate = sessobj(session).delegate;
	if([delegate respondsToSelector:@selector(sessionEndedPlayingTrack:)])
		[delegate performSelectorOnMainThread:@selector(sessionEndedPlayingTrack:)
								   withObject:sessobj(session)
								waitUntilDone:NO];
}

static void streaming_error(sp_session *session, sp_error error)
{
	NSObject<SpotifySessionDelegate> *delegate = sessobj(session).delegate;
	NSError *err = SP_ERROR_OK == error ? nil : makeError(error);
	if([delegate respondsToSelector:@selector(session:streamingError:)])
		[delegate session:sessobj(session) streamingError:err];
}

static void userinfo_updated(sp_session *session)
{
	NSObject<SpotifySessionDelegate> *delegate = sessobj(session).delegate;
	if([delegate respondsToSelector:@selector(sessionUpdatedUserinfo:)])
		[delegate sessionUpdatedUserinfo:sessobj(session)];
}




#pragma mark  
@implementation SpotifySession
#pragma mark -
@synthesize delegate;

-(id)initError:(NSError**)error;
{
	audio_init(&audiofifo);
	
	NSArray *cachesDirs = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
	NSArray *supportDirs = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
	cachesDir = [[cachesDirs objectAtIndex:0] retain];
	supportDir = [[supportDirs objectAtIndex:0] retain];
	
	callbacks = (sp_session_callbacks){
		.logged_in = logged_in,
		.logged_out = logged_out,
		.metadata_updated = metadata_updated,
		.connection_error = connection_error,
		.message_to_user = message_to_user,
		.notify_main_thread = notify_main_thread,
		.music_delivery = music_delivery,
		.play_token_lost = play_token_lost,
		.log_message = log_message,
		.end_of_track = end_of_track,
		.streaming_error = streaming_error,
		.userinfo_updated = userinfo_updated,
	};
	
	config = (sp_session_config){
		.api_version = SPOTIFY_API_VERSION,
		.cache_location = [cachesDir UTF8String],
		.settings_location = [supportDir UTF8String],
		.application_key = g_appkey,
		.application_key_size = g_appkey_size,
		.user_agent = [[[NSProcessInfo processInfo] processName] UTF8String],
		.callbacks = &callbacks,
		.userdata = self,
	};
	
	sp_error sperror = sp_session_create(&config, &session);
	
	if (SP_ERROR_OK != sperror) {
		if(error) *error = makeError(sperror);
		[self release];
		return nil;
	}
	
	[self processEvents];

	
	return self;
}
-(void)dealloc;
{
	// todo: shutdown audiofifo
	[cachesDir release];
	[supportDir release];
	[super dealloc];
}
-(void)shutdown;
{
	[NSObject cancelPreviousPerformRequestsWithTarget:self]; // Cancel any pending processEvents
}

-(void)loginUser:(NSString*)user password:(NSString*)passwd;
{
	sp_session_login(session, [user UTF8String], [passwd UTF8String], 0, NULL);
}
-(void)logout;
{
	sp_session_logout(session);
}

-(SpotifyPlaylist*)starred;
{
	sp_playlist *pl = sp_session_starred_create(session);
	SpotifyPlaylist *playlist = [[[SpotifyPlaylist alloc] initWithPlaylist:pl onSession:session] autorelease];
	sp_playlist_release(pl);
	return playlist;
}

-(void)loadTrack:(SpotifyTrack *)track;
{
	sp_session_player_load(session, track.track);
}
-(void)pause;
{
	audio_fifo_flush(&audiofifo);
	sp_session_player_play(session, FALSE);
}
-(void)play;
{
	audio_fifo_flush(&audiofifo);
	sp_session_player_play(session, TRUE);
}

#pragma mark 
#pragma mark Private
#pragma mark -
-(void)processEvents;
{
	[NSObject cancelPreviousPerformRequestsWithTarget:self selector:_cmd object:nil];
	int msTilNext = 0;
	while(msTilNext == 0)
		sp_session_process_events(session, &msTilNext);
	
	
	NSTimeInterval secsTilNext = msTilNext/1000.;
	[self performSelector:_cmd withObject:nil afterDelay:secsTilNext];
}

-(audio_fifo_t*)audiofifo;
{
	return &audiofifo;
}

-(void)tellDelegateThatSessionLogged:(NSString*)msg;
{
	[delegate session:self logged:msg];
	[msg release];
}

@end
