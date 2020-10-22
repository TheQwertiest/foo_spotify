#include <stdafx.h>

#include "ui_pref_tab_playback.h"

#include <backend/libspotify_backend.h>
#include <backend/spotify_instance.h>
#include <backend/webapi_auth.h>
#include <backend/webapi_auth_scopes.h>
#include <backend/webapi_backend.h>
#include <backend/webapi_objects/webapi_user.h>
#include <fb2k/config.h>
#include <ui/ui_pref_tab_manager.h>

#include <component_urls.h>

#include <qwr/abort_callback.h>
#include <qwr/error_popup.h>
#include <qwr/final_action.h>
#include <qwr/string_helpers.h>

namespace
{

int GetNearestScrollPos( float curValue, int direction )
{
    if ( auto curShift = curValue - std::floor( curValue );
         !curShift )
    {
        return (int)( curValue + direction );
    }
    else
    {
        if ( direction < 0 )
        {
            return (int)std::floor( curValue );
        }
        else if ( direction > 0 )
        {
            return (int)std::ceil( curValue );
        }
        else
        {
            return (int)std::round( curValue );
        }
    }
}

} // namespace

namespace sptf::ui
{

PreferenceTabPlayback::PreferenceTabPlayback( PreferenceTabManager* pParent )
    : pParent_( pParent )
    , trackToEdit_( { { IDC_SLIDER_CACHE_PERCENT, IDC_EDIT_CACHE_PERCENT },
                      { IDC_SLIDER_CACHE_MB, IDC_EDIT_CACHE_MB } } )
    , editToTrack_( { { IDC_EDIT_CACHE_PERCENT, IDC_SLIDER_CACHE_PERCENT },
                      { IDC_EDIT_CACHE_MB, IDC_SLIDER_CACHE_MB } } )
    , enable_normalization_( config::enable_normalization )
    , enable_private_mode_( config::enable_private_mode )
    , libspotify_cache_size_in_percent_( config::libspotify_cache_size_in_percent )
    , libspotify_cache_size_in_mb_( config::libspotify_cache_size_in_mb )
    , preferred_bitrate_(
          config::preferred_bitrate,
          { { config::BitrateSettings::Bitrate96k, 0 },
            { config::BitrateSettings::Bitrate160k, 1 },
            { config::BitrateSettings::Bitrate320k, 2 } } )
    , ddxOptions_(
          { qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_CheckBox>( enable_normalization_, IDC_CHECK_NORMALIZE ),
            qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_CheckBox>( enable_private_mode_, IDC_CHECK_PRIVATE ),
            qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_TrackBar>( libspotify_cache_size_in_percent_, IDC_SLIDER_CACHE_PERCENT ),
            qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_TrackBar>( libspotify_cache_size_in_mb_, IDC_SLIDER_CACHE_MB ),
            qwr::ui::CreateUiDdxOption<qwr::ui::UiDdx_ComboBox>( preferred_bitrate_, IDC_COMBO_BITRATE ) } )
{
}

PreferenceTabPlayback::~PreferenceTabPlayback()
{
    for ( auto& ddxOpt: ddxOptions_ )
    {
        ddxOpt->Option().Revert();
    }
}

HWND PreferenceTabPlayback::CreateTab( HWND hParent )
{
    return Create( hParent );
}

CDialogImplBase& PreferenceTabPlayback::Dialog()
{
    return *this;
}

const wchar_t* PreferenceTabPlayback::Name() const
{
    return L"Playback";
}

t_uint32 PreferenceTabPlayback::get_state()
{
    const bool hasChanged =
        ddxOptions_.cend() != std::find_if( ddxOptions_.cbegin(), ddxOptions_.cend(), []( const auto& ddxOpt ) {
            return ddxOpt->Option().HasChanged();
        } );

    return ( preferences_state::resettable | ( hasChanged ? preferences_state::changed : 0 ) );
}

void PreferenceTabPlayback::apply()
{
    for ( auto& ddxOpt: ddxOptions_ )
    {
        ddxOpt->Option().Apply();
    }

    RefreshLibSpotifySettings();

    pParent_->OnDataChanged();
}

void PreferenceTabPlayback::reset()
{
    for ( auto& ddxOpt: ddxOptions_ )
    {
        ddxOpt->Option().ResetToDefault();
    }
    DoFullDdxToUi();

    pParent_->OnDataChanged();
}

BOOL PreferenceTabPlayback::OnInitDialog( HWND hwndFocus, LPARAM lParam )
{
    CComboBox comboBitrate( GetDlgItem( IDC_COMBO_BITRATE ) );
    comboBitrate.AddString( L"96 kbit/s" );
    comboBitrate.AddString( L"160 kbit/s" );
    comboBitrate.AddString( L"320 kbit/s" );

    CTrackBarCtrl trackCacheMb( GetDlgItem( IDC_SLIDER_CACHE_MB ) );
    trackCacheMb.SetRangeMin( 0 );
    trackCacheMb.SetRangeMax( 8192 );
    trackCacheMb.SetTicFreq( 512 );

    CTrackBarCtrl trackCachePercent( GetDlgItem( IDC_SLIDER_CACHE_PERCENT ) );
    trackCachePercent.SetRangeMin( 0 );
    trackCachePercent.SetRangeMax( 100 );
    trackCachePercent.SetTicFreq( 5 );

    for ( auto& ddxOpt: ddxOptions_ )
    {
        ddxOpt->Ddx().SetHwnd( m_hWnd );
    }
    DoFullDdxToUi();

    this->SetDlgItemInt( trackToEdit_.at( IDC_SLIDER_CACHE_MB ), trackCacheMb.GetPos(), false );
    this->SetDlgItemInt( trackToEdit_.at( IDC_SLIDER_CACHE_PERCENT ), trackCachePercent.GetPos(), false );

    suppressUiDdx_ = false;

    return TRUE; // set focus to default control
}

void PreferenceTabPlayback::OnDdxUiChange( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    if ( suppressUiDdx_ )
    {
        return;
    }

    auto it = std::find_if( ddxOptions_.begin(), ddxOptions_.end(), [nID]( auto& val ) {
        return val->Ddx().IsMatchingId( nID );
    } );

    if ( ddxOptions_.end() != it )
    {
        ( *it )->Ddx().ReadFromUi();
        pParent_->OnDataChanged();
    }
}

void PreferenceTabPlayback::OnDdxValueChange( int nID )
{
    // avoid triggering loopback ddx
    suppressUiDdx_ = true;
    const qwr::final_action autoSuppress( [&] { suppressUiDdx_ = false; } );

    auto it = std::find_if( ddxOptions_.begin(), ddxOptions_.end(), [nID]( auto& val ) {
        return val->Ddx().IsMatchingId( nID );
    } );

    assert( ddxOptions_.end() != it );
    ( *it )->Ddx().WriteToUi();
}

LRESULT PreferenceTabPlayback::OnTrackBarPosChangedNotify( LPNMHDR pnmh )
{
    auto pNMTR = reinterpret_cast<NMTRBTHUMBPOSCHANGING*>( pnmh );
    if ( pNMTR->nReason == TB_ENDTRACK )
    {
        return 1;
    }

    if ( suppressUiDdx_ )
    {
        return 1;
    }

    const auto id = pNMTR->hdr.idFrom;

    CTrackBarCtrl trackBar( GetDlgItem( id ) );
    const auto tickCount = trackBar.GetNumTics() - 1;
    const auto rangeMin = trackBar.GetRangeMin();
    const auto rangeMax = trackBar.GetRangeMax();
    const auto tickFreq = ( rangeMax - rangeMin ) / tickCount;
    const auto prevPos = trackBar.GetPos();
    const auto pos = pNMTR->dwPos;

    auto adjustedPos = [&] {
        const auto ticks = (float)pos / tickFreq;
        const auto diffSign = ( (int)pos - (int)prevPos < 0 ? -1 : 1 );
        switch ( pNMTR->nReason )
        {
        case TB_PAGEUP:
        case TB_PAGEDOWN:
        {
            const auto diffTicks = std::abs( (int)pos - (int)prevPos ) / (int)tickFreq;
            return ( GetNearestScrollPos( ticks, diffSign ) + diffSign * diffTicks ) * (int)tickFreq;
        }
        case TB_LINEUP:
        case TB_LINEDOWN:
        {
            return GetNearestScrollPos( ticks, diffSign ) * (int)tickFreq;
        }
        case TB_THUMBTRACK:
        case TB_THUMBPOSITION:
        { // snap to the closest
            return GetNearestScrollPos( ticks, 0 ) * (int)tickFreq;
        }
        default:
        {
            return (int)pos;
        }
        }
    }();
    adjustedPos = std::clamp( adjustedPos, rangeMin, rangeMax );

    tracksRedrawing_.emplace( id );
    trackBar.SetPos( adjustedPos );
    OnDdxUiChange( 0, id, nullptr );

    // avoid triggering ddx for linked edit
    suppressUiDdx_ = true;
    const qwr::final_action autoSuppress( [&] { suppressUiDdx_ = false; } );

    this->SetDlgItemInt( trackToEdit_.at( id ), adjustedPos, false );

    return 1;
}

void PreferenceTabPlayback::OnTrackBarHScroll( UINT nSBCode, UINT nPos, CTrackBarCtrl trackBar )
{
    if ( nSBCode == TB_ENDTRACK )
    {
        return;
    }

    const auto trackId = trackBar.GetDlgCtrlID();
    if ( tracksRedrawing_.count( trackId ) )
    {
        tracksRedrawing_.erase( trackId );
        return;
    }

    // some events are not processed by TRBN_THUMBPOSCHANGING

    const auto pos = trackBar.GetPos();
    OnDdxUiChange( 0, trackId, nullptr );

    // avoid triggering ddx for linked edit
    suppressUiDdx_ = true;
    const qwr::final_action autoSuppress( [&] { suppressUiDdx_ = false; } );

    this->SetDlgItemInt( trackToEdit_.at( trackId ), pos, false );
}

void PreferenceTabPlayback::OnTrackBarEdit( UINT uNotifyCode, int nID, CWindow wndCtl )
{
    if ( suppressUiDdx_ )
    {
        return;
    }

    BOOL lpTrans;
    const auto editPosRaw = this->GetDlgItemInt( nID, &lpTrans, false );
    if ( !lpTrans )
    {
        return;
    }

    const auto trackId = editToTrack_.at( nID );
    CTrackBarCtrl trackbar( GetDlgItem( trackId ) );
    const auto editPos = std::clamp<int>( editPosRaw, trackbar.GetRangeMin(), trackbar.GetRangeMax() );
    if ( editPos != editPosRaw )
    {
        this->SetDlgItemInt( nID, editPos, false );
    }

    if ( trackbar.GetPos() == editPos )
    {
        return;
    }

    switch ( trackId )
    {
    case IDC_SLIDER_CACHE_MB:
        libspotify_cache_size_in_mb_ = editPos;
        break;
    case IDC_SLIDER_CACHE_PERCENT:
        libspotify_cache_size_in_percent_ = editPos;
        break;
    default:
        assert( false );
        break;
    }

    OnDdxValueChange( trackId );
}

void PreferenceTabPlayback::DoFullDdxToUi()
{
    if ( !this->m_hWnd )
    {
        return;
    }

    for ( auto& ddxOpt: ddxOptions_ )
    {
        ddxOpt->Ddx().WriteToUi();
    }
}

void PreferenceTabPlayback::RefreshLibSpotifySettings()
{
    auto& lsBackend = SpotifyInstance::Get().GetLibSpotify_Backend();
    lsBackend.RefreshBitrate();
    lsBackend.RefreshNormalization();
    lsBackend.RefreshPrivateMode();
    lsBackend.RefreshCacheSize();
}

} // namespace sptf::ui
