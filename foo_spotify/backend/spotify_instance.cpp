#include <stdafx.h>

#include "spotify_instance.h"

#include <backend/libspotify_backend.h>
#include <backend/webapi_auth.h>
#include <backend/webapi_backend.h>
#include <fb2k/playback.h>
#include <utils/abort_manager.h>

#include <qwr/abort_callback.h>

namespace sptf
{

SpotifyInstance& SpotifyInstance::Get()
{
    static SpotifyInstance si;
    return si;
}

void SpotifyInstance::Finalize()
{
    std::lock_guard lg( mutex_ );
    isFinalized_ = true;

    const auto finalize = []( auto& pElem ) {
        if ( pElem )
        {
            try
            {
                pElem->Finalize();
            }
            catch ( const std::exception& )
            {
            }
            pElem.reset();
        }
    };

    if ( fb2k_playCallbacks_initialized_ )
    {
        fb2k::PlayCallbacks::Finalize();
        fb2k_playCallbacks_initialized_ = false;
    }

    finalize( pWebApi_backend_ );
    finalize( pLibSpotify_backend_ );
    finalize( pAbortManager_ );
}

AbortManager& SpotifyInstance::GetAbortManager()
{
    InitializeAll();
    assert( pAbortManager_ );
    return *pAbortManager_;
}

LibSpotify_Backend& SpotifyInstance::GetLibSpotify_Backend()
{
    InitializeAll();
    assert( pLibSpotify_backend_ );
    return *pLibSpotify_backend_;
}

WebApi_Backend& SpotifyInstance::GetWebApi_Backend()
{
    InitializeAll();
    assert( pWebApi_backend_ );
    return *pWebApi_backend_;
}

void SpotifyInstance::InitializeAll()
{
    std::lock_guard lg( mutex_ );
    if ( isFinalized_ )
    {
        throw qwr::QwrException( "foobar2000 is exiting" );
    }

    if ( !pAbortManager_ )
    {
        pAbortManager_ = std::make_unique<AbortManager>();
    }
    if ( !pLibSpotify_backend_ )
    {
        pLibSpotify_backend_ = std::make_unique<LibSpotify_Backend>( *pAbortManager_ );
    }
    if ( !pWebApi_backend_ )
    {
        pWebApi_backend_ = std::make_unique<WebApi_Backend>( *pAbortManager_ );
    }
    if ( !fb2k_playCallbacks_initialized_ )
    {
        fb2k::PlayCallbacks::Initialize( *pLibSpotify_backend_ );
        fb2k_playCallbacks_initialized_ = true;
    }
}

} // namespace sptf