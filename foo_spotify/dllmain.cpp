#include <stdafx.h>

#include <backend/libspotify_backend.h>
#include <backend/webapi_backend.h>

#include <qwr/delayed_executor.h>
#include <qwr/error_popup.h>

DECLARE_COMPONENT_VERSION( SPTF_NAME, SPTF_VERSION, SPTF_ABOUT );

VALIDATE_COMPONENT_FILENAME( SPTF_DLL_NAME );

using namespace sptf;

namespace
{

class sptf_initquit : public initquit
{
public:
    void on_init() override
    {
        qwr::DelayedExecutor::GetInstance().EnableExecution(); ///< Enable task processing (e.g. error popups)
        try
        {
            sptf::WebApiBackend::Instance().Initialize();
            sptf::LibSpotifyBackend::Instance().Initialize();
        }
        catch ( const std::exception& e )
        {
            qwr::ReportErrorWithPopup( e.what(), SPTF_UNDERSCORE_NAME );
        }
    }

    void on_quit() override
    { // Careful when changing invocation order here!
        sptf::LibSpotifyBackend::Instance().Finalize();
        sptf::WebApiBackend::Instance().Finalize();
    }
};

initquit_factory_t<sptf_initquit> g_initquit;

} // namespace

extern "C" BOOL WINAPI DllMain( HINSTANCE ins, DWORD reason, [[maybe_unused]] LPVOID lp )
{
    switch ( reason )
    {
    case DLL_PROCESS_ATTACH:
    {
        break;
    }
    case DLL_PROCESS_DETACH:
    {
        break;
    }
    default:
        break;
    }

    return TRUE;
}
