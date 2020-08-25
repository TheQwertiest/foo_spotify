#pragma once

#include <vector>

namespace sptf
{

template <class T>
class SecureAllocator
{
public:
    static_assert( !std::is_const<T>::value,
                   "The C++ Standard forbids containers of const elements "
                   "because allocator<const T> is ill-formed." );

    using value_type = T;
    using propagate_on_container_move_assignment = std::true_type;
    using is_always_equal = std::true_type;

    constexpr SecureAllocator() noexcept
    {
    }

    constexpr SecureAllocator( const SecureAllocator& ) noexcept = default;

    template <class OtherT>
    constexpr SecureAllocator( const SecureAllocator<OtherT>& ) noexcept
    {
    }

    void deallocate( T* const ptr, const size_t count )
    {
        SecureZeroMemory( reinterpret_cast<uint8_t*>( ptr ), count * sizeof( T ) );
        baseAllocator_.deallocate( ptr, count );
    }

    [[nodiscard]] T* allocate( const size_t count )
    {
        return baseAllocator_.allocate( count );
    }

private:
    std::allocator<T> baseAllocator_;
};

template <class T, class OtherT>
[[nodiscard]] inline bool operator==( const SecureAllocator<T>&,
                                      const SecureAllocator<OtherT>& ) noexcept
{
    return true;
}

template <class T, class OtherT>
[[nodiscard]] inline bool operator!=( const SecureAllocator<T>&,
                                      const SecureAllocator<OtherT>& ) noexcept
{
    return false;
}

template <class T>
using SecureVector = std::vector<T, SecureAllocator<T>>;

} // namespace sptf
