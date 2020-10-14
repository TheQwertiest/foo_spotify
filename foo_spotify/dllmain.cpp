#include <stdafx.h>

#include <backend/spotify_instance.h>
#include <ui/ui_not_auth.h>

#include <qwr/abort_callback.h>
#include <qwr/delayed_executor.h>
#include <qwr/error_popup.h>

DECLARE_COMPONENT_VERSION( SPTF_NAME, SPTF_VERSION, SPTF_ABOUT );

VALIDATE_COMPONENT_FILENAME( SPTF_DLL_NAME );

using namespace sptf;

namespace
{

bool HasComponent( const std::u8string& componentName )
{
    pfc::string8_fast temp;
    for ( service_enum_t<componentversion> e; !e.finished(); ++e )
    {
        auto cv = e.get();
        cv->get_file_name( temp );
        if ( temp.c_str() == componentName )
        {
            return true;
        }
    }

    return false;
}

} // namespace

namespace
{

class sptf_initquit : public initquit
{
public:
    void on_init() override
    {
        qwr::DelayedExecutor::GetInstance().EnableExecution(); ///< Enable task processing (e.g. error popups)
        if ( HasComponent( "foo_input_spotify" ) )
        {
            qwr::ReportErrorWithPopup( SPTF_UNDERSCORE_NAME,
                                       "!!!        `foo_input_spotify` component detected!             !!!\n"
                                       "!!! It is incompatible with `foo_spotify` and must be removed! !!!\n"
                                       "!!!           Otherwise foobar2000 *will* crash!               !!!" );
        }
    }

    void on_quit() override
    { // Careful when changing invocation order here!
        qwr::GlobalAbortCallback::GetInstance().Abort();
        SpotifyInstance::Get().Finalize();
        ui::NotAuthHandler::Get().CloseDialog();
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
