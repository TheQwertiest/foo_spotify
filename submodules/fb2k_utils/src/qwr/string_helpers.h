#pragma once

#include <charconv>
#include <optional>
#include <string>
#include <string_view>

namespace qwr::string
{

template <typename T>
std::basic_string_view<T> TrimView( std::basic_string_view<T> str )
{
    size_t first = str.find_first_not_of( ' ' );
    if ( std::string::npos == first )
    {
        return str;
    }
    size_t last = str.find_last_not_of( ' ' );
    return str.substr( first, ( last - first + 1 ) );
}

template <typename T>
std::basic_string<T> Trim( std::basic_string_view<T> str )
{
    const auto view = TrimView( str );
    return std::basic_string<T>{ view.data(), view.size() };
}

std::vector<std::u8string_view> SplitByLines( std::u8string_view str );

template <typename T>
std::vector<std::basic_string_view<T>> Split( std::basic_string_view<T> str, const std::basic_string<T>& separator )
{
    return ranges::views::split( str, separator )
           | ranges::views::transform( []( auto&& rng ) {
                 return std::basic_string_view<T>{ &*rng.begin(), static_cast<size_t>( ranges::distance( rng ) ) };
             } )
           | ranges::to_vector;
}

template <typename T>
std::vector<std::basic_string_view<T>> Split( std::basic_string_view<T> str, T separator )
{
    return Split( str, std::basic_string<T>( 1, separator ) );
}

template <typename T, typename ContT>
std::basic_string<T> Join( const ContT& cont, T ch )
{
    static_assert( std::is_same_v<typename ContT::value_type, std::basic_string<T>> || std::is_same_v<typename ContT::value_type, std::basic_string_view<T>> || std::is_same_v<typename ContT::value_type, const T*> );

    using string_t = std::basic_string<T>;

    string_t words_str;
    words_str.reserve( cont.size() * 6 );
    for ( const auto& word: cont )
    {
        if constexpr ( std::is_constructible_v<string_t, typename ContT::value_type> )
        {
            words_str += word;
        }
        else
        {
            words_str += string_t{ word.data(), word.size() };
        }
        words_str += ch;
    }
    if ( !words_str.empty() )
    {
        words_str.resize( words_str.size() - 1 );
    }
    return words_str;
}

template <typename T, typename T2>
std::optional<T> GetNumber( std::basic_string_view<T2> strView, int base = 10 )
{
    T number;
    if ( auto [pos, ec] = std::from_chars( strView.data(), strView.data() + strView.size(), number, base );
         ec == std::errc{} )
    {
        return number;
    }
    else
    {
        return std::nullopt;
    }
}

} // namespace qwr::string
