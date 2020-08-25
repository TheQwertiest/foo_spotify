#include <stdafx.h>

#include "webapi_cache.h"

#include <backend/spotify_instance.h>
#include <utils/abort_manager.h>

#include <qwr/winapi_error_helpers.h>

// RODO: move to props
#pragma comment( lib, "Urlmon.lib" )

namespace fs = std::filesystem;

namespace
{

using namespace sptf;

class DownloadStatus : public IBindStatusCallback
{
public:
    DownloadStatus( abort_callback& abort );

public:
    STDMETHOD( OnStartBinding )(
        /* [in] */ DWORD dwReserved,
        /* [in] */ IBinding __RPC_FAR* pib )
    {
        return E_NOTIMPL;
    }

    STDMETHOD( GetPriority )(
        /* [out] */ LONG __RPC_FAR* pnPriority )
    {
        return E_NOTIMPL;
    }

    STDMETHOD( OnLowResource )(
        /* [in] */ DWORD reserved )
    {
        return E_NOTIMPL;
    }

    STDMETHOD( OnProgress )(
        /* [in] */ ULONG ulProgress,
        /* [in] */ ULONG ulProgressMax,
        /* [in] */ ULONG ulStatusCode,
        /* [in] */ LPCWSTR wszStatusText );

    STDMETHOD( OnStopBinding )(
        /* [in] */ HRESULT hresult,
        /* [unique][in] */ LPCWSTR szError )
    {
        return E_NOTIMPL;
    }

    STDMETHOD( GetBindInfo )(
        /* [out] */ DWORD __RPC_FAR* grfBINDF,
        /* [unique][out][in] */ BINDINFO __RPC_FAR* pbindinfo )
    {
        return E_NOTIMPL;
    }

    STDMETHOD( OnDataAvailable )(
        /* [in] */ DWORD grfBSCF,
        /* [in] */ DWORD dwSize,
        /* [in] */ FORMATETC __RPC_FAR* pformatetc,
        /* [in] */ STGMEDIUM __RPC_FAR* pstgmed )
    {
        return E_NOTIMPL;
    }

    STDMETHOD( OnObjectAvailable )(
        /* [in] */ REFIID riid,
        /* [iid_is][in] */ IUnknown __RPC_FAR* punk )
    {
        return E_NOTIMPL;
    }

    // IUnknown methods.  Note that IE never calls any of these methods, since
    // the caller owns the IBindStatusCallback interface, so the methods all
    // return zero/E_NOTIMPL.

    STDMETHOD_( ULONG, AddRef )
    ()
    {
        return 0;
    }

    STDMETHOD_( ULONG, Release )
    ()
    {
        return 0;
    }

    STDMETHOD( QueryInterface )(
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void __RPC_FAR* __RPC_FAR* ppvObject )
    {
        return E_NOTIMPL;
    }

private:
    std::unique_ptr<AbortManager::AbortableScope> pScope_;
    std::mutex mutex_;
    bool hasAborted_ = false;
};

DownloadStatus::DownloadStatus( abort_callback& abort )
{
    auto& am = SpotifyInstance::Get().GetAbortManager();
    pScope_ = std::make_unique<AbortManager::AbortableScope>(
        am.GetAbortableScope( [&] {
            std::lock_guard lg( mutex_ );
            hasAborted_ = true;
        },
                              abort ) );
}

HRESULT DownloadStatus::OnProgress( ULONG ulProgress, ULONG ulProgressMax,
                                    ULONG ulStatusCode, LPCWSTR wszStatusText )
{
    std::lock_guard lg( mutex_ );
    return ( hasAborted_ ? E_ABORT : S_OK );
}

} // namespace

namespace sptf
{

WebApi_ImageCache::WebApi_ImageCache( const std::string& cacheSubdir )
    : cacheSubdir_( cacheSubdir )
{
}

fs::path WebApi_ImageCache::GetImage( const std::string& id, const std::string& imgUrl, abort_callback& abort )
{
    std::lock_guard lock( cacheMutex_ );

    const auto imagePath = path::WebApiCache() / "images" / cacheSubdir_ / fmt::format( "{}.jpeg", id );
    if ( !fs::exists( imagePath ) )
    {
        fs::create_directories( imagePath.parent_path() );

        const auto url_w = qwr::unicode::ToWide( imgUrl );

        DownloadStatus ds( abort );
        auto hr = URLDownloadToFile( nullptr, url_w.c_str(), imagePath.c_str(), 0, &ds );
        qwr::error::CheckHR( hr, "URLDownloadToFile" );
    }
    assert( fs::exists( imagePath ) );
    return imagePath;
}

} // namespace sptf