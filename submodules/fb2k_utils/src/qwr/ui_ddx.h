#pragma once

#include <nonstd/span.hpp>

namespace qwr::ui
{

class IUiDdx
{
public:
    IUiDdx() = default;
    virtual ~IUiDdx() = default;

    virtual bool IsMatchingId( int controlId ) const = 0;

    virtual void SetHwnd( HWND hWnd ) = 0;
    virtual void ReadFromUi() = 0;
    virtual void WriteToUi() = 0;
};

template <typename T>
class UiDdx_CheckBox final
    : public IUiDdx
{
public:
    using value_type = typename T;

public:
    UiDdx_CheckBox( T& value, int controlId )
        : value_( value )
        , controlId_( controlId )
    {
        static_assert( std::is_convertible_v<T, bool> );
        static_assert( std::is_assignable_v<T&, bool> );
    }
    ~UiDdx_CheckBox() override = default;

    bool IsMatchingId( int controlId ) const override
    {
        return ( controlId == controlId_ );
    }

    void SetHwnd( HWND hWnd ) override
    {
        hWnd_ = hWnd;
    }

    void ReadFromUi() override
    {
        if ( !hWnd_ )
        {
            return;
        }

        value_ = uButton_GetCheck( hWnd_, controlId_ );
    }
    void WriteToUi() override
    {
        if ( !hWnd_ )
        {
            return;
        }

        uButton_SetCheck( hWnd_, controlId_, static_cast<bool>( value_ ) );
    }

private:
    T& value_;
    HWND hWnd_ = nullptr;
    const int controlId_;
};

template <typename T>
class UiDdx_TextEdit final
    : public IUiDdx
{
public:
    using value_type = typename T;

public:
    UiDdx_TextEdit( T& value, int controlId )
        : value_( value )
        , controlId_( controlId )
    {
        static_assert( std::is_convertible_v<T, std::u8string> );
        static_assert( std::is_assignable_v<T&, std::u8string> );
    }
    ~UiDdx_TextEdit() override = default;

    bool IsMatchingId( int controlId ) const override
    {
        return ( controlId == controlId_ );
    }

    void SetHwnd( HWND hWnd ) override
    {
        hWnd_ = hWnd;
    }

    void ReadFromUi() override
    {
        if ( !hWnd_ )
        {
            return;
        }

        value_ = smp::pfc_x::uGetDlgItemText<char8_t>( hWnd_, controlId_ );
    }
    void WriteToUi() override
    {
        if ( !hWnd_ )
        {
            return;
        }

        uSetDlgItemText( hWnd_, controlId_, static_cast<std::u8string>( value_ ).c_str() );
    }

private:
    T& value_;
    HWND hWnd_ = nullptr;
    const int controlId_;
};

template <typename T>
class UiDdx_RadioRange final
    : public IUiDdx
{
public:
    using value_type = typename T;

public:
    UiDdx_RadioRange( T& value, nonstd::span<const int> controlIdList )
        : value_( value )
        , controlIdList_( controlIdList.begin(), controlIdList.end() )
    {
        static_assert( std::is_convertible_v<T, int> );
        static_assert( std::is_assignable_v<T&, int> );
    }
    ~UiDdx_RadioRange() override = default;

    bool IsMatchingId( int controlId ) const override
    {
        return ( controlIdList_.cend() != ranges::find( controlIdList_, controlId ) );
    }

    void SetHwnd( HWND hWnd ) override
    {
        hWnd_ = hWnd;
    }

    void ReadFromUi() override
    {
        if ( !hWnd_ )
        {
            return;
        }

        for ( const auto& id: controlIdList_ )
        {
            if ( uButton_GetCheck( hWnd_, id ) )
            {
                value_ = id;
                break;
            }
        }
    }
    void WriteToUi() override
    {
        if ( !hWnd_ )
        {
            return;
        }

        for ( const auto& id: controlIdList_ )
        {
            uButton_SetCheck( hWnd_, id, static_cast<int>( value_ ) == id );
        }
    }

private:
    T& value_;
    HWND hWnd_ = nullptr;
    const std::vector<int> controlIdList_;
};

template <typename ListT, typename T>
class UiDdx_ListBase final
    : public IUiDdx
{
public:
    using value_type = typename T;

public:
    UiDdx_ListBase( T& value, int controlId )
        : value_( value )
        , controlId_( controlId )
    {
        static_assert( std::is_convertible_v<T, int> );
        static_assert( std::is_assignable_v<T&, int> );
    }
    ~UiDdx_ListBase() override = default;

    bool IsMatchingId( int controlId ) const override
    {
        return ( controlId == controlId_ );
    }

    void SetHwnd( HWND hWnd ) override
    {
        hWnd_ = hWnd;
    }

    void ReadFromUi() override
    {
        if ( !hWnd_ )
        {
            return;
        }

        value_ = ListT{ ::GetDlgItem( hWnd_, controlId_ ) }.GetCurSel();
    }
    void WriteToUi() override
    {
        if ( !hWnd_ )
        {
            return;
        }

        ListT{ ::GetDlgItem( hWnd_, controlId_ ) }.SetCurSel( static_cast<int>( value_ ) );
    }

private:
    T& value_;
    HWND hWnd_ = nullptr;
    const int controlId_;
};

template <typename T>
using UiDdx_ComboBox = UiDdx_ListBase<CComboBox, T>;

template <typename T>
using UiDdx_ListBox = UiDdx_ListBase<CListBox, T>;

template <template <typename> typename DdxT, typename T, typename... Args>
std::unique_ptr<IUiDdx> CreateUiDdx( T& value, Args&&... args )
{
    return std::make_unique<DdxT<T>>( value, std::forward<Args>( args )... );
}

} // namespace qwr::ui
