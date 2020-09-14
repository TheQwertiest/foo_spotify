#include <memory>

namespace nlohmann
{
template <typename T>
struct adl_serializer<std::unique_ptr<T>>
{
    static void to_json( nlohmann::json& j, const std::unique_ptr<T>& value )
    {
        if ( !value )
        {
            throw nlohmann::json::type_error::create( 302, "type must not be null, but is" );
        }

        j = *value;
    }

    static void from_json( const nlohmann::json& j, std::unique_ptr<T>& p )
    {
        if ( j.is_null() )
        {
            throw nlohmann::json::type_error::create( 302, "type must not be null, but is" );
        }

        p = std::make_unique<T>( j.get<T>() );
    }
};

template <typename T>
struct adl_serializer<std::shared_ptr<T>>
{
    static void to_json( nlohmann::json& j, const std::shared_ptr<T>& value )
    {
        if ( !value )
        {
            throw nlohmann::json::type_error::create( 302, "type must not be null, but is" );
        }

        j = *value;
    }

    static void from_json( const nlohmann::json& j, std::shared_ptr<T>& p )
    {
        if ( j.is_null() )
        {
            throw nlohmann::json::type_error::create( 302, "type must not be null, but is" );
        }

        p = std::make_shared<T>( j.get<T>() );
    }
};

} // namespace nlohmann
