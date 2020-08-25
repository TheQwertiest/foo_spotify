#pragma once

#include <backend/libspotify_backend.h>

#include <libspotify/api.h>

namespace sptf::wrapper
{

namespace internal
{

template <typename T>
struct SpotifyTraits
{
    static_assert( "unknown specialization" );
};

template <>
struct SpotifyTraits<sp_track>
{
    static void AddRef( sp_track* album )
    {
        sp_track_add_ref( album );
    }

    static void Release( sp_track* album )
    {
        sp_track_release( album );
    }

    static bool IsLoaded( sp_track* album )
    {
        return sp_track_is_loaded( album );
    }
};

template <>
struct SpotifyTraits<sp_link>
{
    static void AddRef( sp_link* link )
    {
        sp_link_add_ref( link );
    }

    static void Release( sp_link* link )
    {
        sp_link_release( link );
    }

    // does not have a "IsLoaded" method
};

} // namespace internal

template <typename T>
class Ptr
{
public:
    Ptr() = default;

    Ptr( T* ptr )
        : ptr_( ptr )
    {

        if ( ptr_ )
        {
            internal::SpotifyTraits<T>::AddRef( ptr_ );
        }
    }

    Ptr( const Ptr<T*>& other )
        : ptr_( other.ptr_ )
    {
        if ( ptr_ )
        {
            internal::SpotifyTraits<T>::AddRef( ptr_ );
        }
    }

    Ptr( Ptr<T*>&& other )
        : ptr_( other.ptr_ )
    {
        other.ptr_ = nullptr;
    }

    void Release()
    {
        auto ptr = ptr_;
        ptr_ = nullptr;
        if ( ptr )
        {
            internal::SpotifyTraits<T>::Release( ptr );
        }
    }

    void Attach( T* ptr )
    {
        if ( ptr_ )
        {
            internal::SpotifyTraits<T>::Release( ptr );
        }
        ptr_ = ptr;
    }

    operator bool() const
    {
        return !!ptr_;
    }

    bool operator!() const
    {
        return !ptr_;
    }

    T* operator=( T* ptr )
    {
        Release();
        ptr_ = ptr;
        if ( ptr_ )
        {
            internal::SpotifyTraits<T>::AddRef( ptr_ );
        }
        return ptr_;
    }

    operator T*() const
    {
        return ptr_;
    }

private:
    T* ptr_ = nullptr;
};

} // namespace sptf::wrapper
