#include <stdafx.h>

#include <backend/spotify_object.h>

using namespace std::literals::string_view_literals;

using namespace sptf;

namespace
{

class FilesystemSpotify : public foobar2000_io::filesystem
{
public:
    bool get_canonical_path( const char* p_path, pfc::string_base& p_out ) override;

    bool is_our_path( const char* p_path ) override;
    bool get_display_path( const char* p_path, pfc::string_base& p_out ) override;

    void open( service_ptr_t<file>& p_out, const char* p_path, t_open_mode p_mode, abort_callback& p_abort ) override;

    void remove( const char* p_path, abort_callback& p_abort ) override;
    //! Moves/renames a file. Will fail if the destination file already exists. \n
    //! Note that this function may throw exception_io_denied instead of exception_io_sharing_violation when the file is momentarily in use, due to bugs in Windows MoveFile() API. There is no legitimate way for us to distinguish between the two scenarios.
    void move( const char* p_src, const char* p_dst, abort_callback& p_abort ) override;
    //! Queries whether a file at specified path belonging to this filesystem is a remote object or not.
    bool is_remote( const char* p_src ) override;

    //! Retrieves stats of a file at specified path.
    void get_stats( const char* p_path, t_filestats& p_stats, bool& p_is_writeable, abort_callback& p_abort ) override;

    //! Creates a directory.
    void create_directory( const char* p_path, abort_callback& p_abort ) override;
    void list_directory( const char* p_path, directory_callback& p_out, abort_callback& p_abort ) override;

    //! Hint; returns whether this filesystem supports mime types. \n
    //! When this returns false, all file::get_content_type() calls on files opened thru this filesystem implementation will return false; otherwise, file::get_content_type() calls may return true depending on the file.
    bool supports_content_types() override;
    ;
};

} // namespace

namespace
{

bool FilesystemSpotify::get_canonical_path( const char* p_path, pfc::string_base& p_out )
{
    if ( !SpotifyFilteredTrack::IsValid( p_path, true ) )
    {
        return false;
    }

    try
    {
        const auto ret = SpotifyObject( p_path ).ToSchema();
        p_out = ret.c_str();
        return true;
    }
    catch ( qwr::QwrException& )
    {
        return false;
    }
}

bool FilesystemSpotify::is_our_path( const char* p_path )
{
    // accept only filtered input
    return SpotifyFilteredTrack::IsValid( p_path, true );
}

bool FilesystemSpotify::get_display_path( const char* p_path, pfc::string_base& p_out )
{
    if ( !SpotifyFilteredTrack::IsValid( p_path, true ) )
    {
        return false;
    }

    try
    {
        const auto ret = SpotifyObject( p_path ).ToUri();
        p_out = ret.c_str();
        return true;
    }
    catch ( qwr::QwrException& )
    {
        return false;
    }
}

void FilesystemSpotify::open( service_ptr_t<file>& p_out, const char* p_path, t_open_mode p_mode, abort_callback& p_abort )
{
    (void)p_abort;

    if ( p_mode != open_mode_read )
    {
        throw pfc::exception( "Can't modify remote files" );
    }

    p_out = ::fb2k::service_new<file_streamstub>();
}

void FilesystemSpotify::remove( const char* p_path, abort_callback& p_abort )
{
    (void)p_path;
    (void)p_abort;
    throw pfc::exception( "Can't delete remote files" );
}

void FilesystemSpotify::move( const char* p_src, const char* p_dst, abort_callback& p_abort )
{
    (void)p_src;
    (void)p_dst;
    (void)p_abort;
    throw pfc::exception( "Can't move remote files" );
}

bool FilesystemSpotify::is_remote( const char* p_src )
{
    (void)p_src;
    return false;
}

void FilesystemSpotify::get_stats( const char* p_path, t_filestats& p_stats, bool& p_is_writeable, abort_callback& p_abort )
{
    (void)p_abort;
    p_stats = filestats_invalid;
    p_is_writeable = false;
}

void FilesystemSpotify::create_directory( const char* p_path, abort_callback& p_abort )
{
    (void)p_path;
    (void)p_abort;
    throw pfc::exception( "Directories are not supported" );
}

void FilesystemSpotify::list_directory( const char* p_path, directory_callback& p_out, abort_callback& p_abort )
{
    (void)p_path;
    (void)p_out;
    (void)p_abort;
    throw pfc::exception( "Directories are not supported" );
}

bool FilesystemSpotify::supports_content_types()
{
    return false;
}

} // namespace

namespace
{

static service_factory_single_t<FilesystemSpotify> g_filesystem;

}