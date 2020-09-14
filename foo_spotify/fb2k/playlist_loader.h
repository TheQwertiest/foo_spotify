#pragma once

namespace sptf
{

class WebApi_Backend;

class PlaylistLoaderSpotify : public playlist_loader
{
public:
    PlaylistLoaderSpotify();

public:
    // playlist_loader

    //! Parses specified playlist file into list of playable locations. Throws exception_io or derivatives on failure, exception_aborted on abort. If specified file is not a recognized playlist file, exception_io_unsupported_format is thrown.
    //! @param p_path Path of playlist file to parse. Used for relative path handling purposes (p_file parameter is used for actual file access).
    //! @param p_file File interface to use for reading. Read/write pointer must be set to beginning by caller before calling.
    //! @param p_callback Callback object receiving enumerated playable item locations.
    void open( const char* p_path, const service_ptr_t<file>& p_file, playlist_loader_callback::ptr p_callback, abort_callback& p_abort ) override;
    //! Writes a playlist file containing specific item list to specified file. Will fail (pfc::exception_not_implemented) if specified playlist_loader is read-only (can_write() returns false).
    //! @param p_path Path of playlist file to write. Used for relative path handling purposes (p_file parameter is used for actual file access).
    //! @param p_file File interface to use for writing. Caller should ensure that the file is empty (0 bytes long) before calling.
    //! @param p_data List of items to save to playlist file.
    //! @param p_abort abort_callback object signaling user aborting the operation. Note that aborting a save playlist operation will most likely leave user with corrupted/incomplete file.
    void write( const char* p_path, const service_ptr_t<file>& p_file, metadb_handle_list_cref p_data, abort_callback& p_abort ) override;
    //! Returns extension of file format handled by this playlist_loader implementation (a UTF-8 encoded null-terminated string).
    const char* get_extension() override;
    //! Returns whether this playlist_loader implementation supports writing. If can_write() returns false, all write() calls will fail.
    bool can_write() override;
    //! Returns whether specified content type is one of playlist types supported by this playlist_loader implementation or not.
    //! @param p_content_type Content type to query, a UTF-8 encoded null-terminated string.
    bool is_our_content_type( const char* p_content_type ) override;
    //! Returns whether playlist format extension supported by this implementation should be listed on file types associations page.
    bool is_associatable() override;

    /*
    //! Attempts to load a playlist file from specified filesystem path. Throws exception_io or derivatives on failure, exception_aborted on abort. If specified file is not a recognized playlist file, exception_io_unsupported_format is thrown. \n
    //! Equivalent to g_load_playlist_filehint(NULL,p_path,p_callback).
    //! @param p_path Filesystem path to load playlist from, a UTF-8 encoded null-terminated string.
    //! @param p_callback Callback object receiving enumerated playable item locations as well as signaling user aborting the operation.
    static void g_load_playlist( const char* p_path, playlist_loader_callback::ptr p_callback, abort_callback& p_abort );

    //! Attempts to load a playlist file from specified filesystem path. Throws exception_io or derivatives on failure, exception_aborted on abort. If specified file is not a recognized playlist file, exception_io_unsupported_format is thrown.
    //! @param p_path Filesystem path to load playlist from, a UTF-8 encoded null-terminated string.
    //! @param p_callback Callback object receiving enumerated playable item locations as well as signaling user aborting the operation.
    //! @param fileHint File object to read from, can be NULL if not available.
    static void g_load_playlist_filehint( file::ptr fileHint, const char* p_path, playlist_loader_callback::ptr p_callback, abort_callback& p_abort );

    //! Attempts to load a playlist file from specified filesystem path. Throws exception_io or derivatives on failure, exception_aborted on abort. If specified file is not a recognized playlist file, returns false; returns true upon successful playlist load.
    //! @param p_path Filesystem path to load playlist from, a UTF-8 encoded null-terminated string.
    //! @param p_callback Callback object receiving enumerated playable item locations as well as signaling user aborting the operation.
    //! @param fileHint File object to read from, can be NULL if not available.
    static bool g_try_load_playlist( file::ptr fileHint, const char* p_path, playlist_loader_callback::ptr p_callback, abort_callback& p_abort );

    //! Saves specified list of locations into a playlist file. Throws exception_io or derivatives on failure, exception_aborted on abort.
    //! @param p_path Filesystem path to save playlist to, a UTF-8 encoded null-terminated string.
    //! @param p_data List of items to save to playlist file.
    //! @param p_abort abort_callback object signaling user aborting the operation. Note that aborting a save playlist operation will most likely leave user with corrupted/incomplete file.
    static void g_save_playlist( const char* p_path, metadb_handle_list_cref p_data, abort_callback& p_abort );

    //! Processes specified path to generate list of playable items. Includes recursive directory/archive enumeration. \n
    //! Does not touch playlist files encountered - use g_process_path_ex() if specified path is possibly a playlist file; playlist files found inside directories or archives are ignored regardless.\n
    //! Warning: caller must handle exceptions which will occur in case of I/O failure.
    //! @param p_path Filesystem path to process; a UTF-8 encoded null-terminated string.
    //! @param p_callback Callback object receiving enumerated playable item locations as well as signaling user aborting the operation.
    //! @param p_type Origin of p_path string. Reserved for internal use in recursive calls, should be left at default value; it controls various internal behaviors.
    static void g_process_path( const char* p_path, playlist_loader_callback::ptr p_callback, abort_callback& p_abort, playlist_loader_callback::t_entry_type p_type = playlist_loader_callback::entry_user_requested );

    //! Calls attempts to process specified path as a playlist; if that fails (i.e. not a playlist), calls g_process_path with same parameters. See g_process_path for parameter descriptions. \n
    //! Warning: caller must handle exceptions which will occur in case of I/O failure or playlist parsing failure.
    //! @returns True if specified path was processed as a playlist file, false otherwise (relevant in some scenarios where output is sorted after loading, playlist file contents should not be sorted).
    static bool g_process_path_ex( const char* p_path, playlist_loader_callback::ptr p_callback, abort_callback& p_abort, playlist_loader_callback::t_entry_type p_type = playlist_loader_callback::entry_user_requested );
    */

private:
    WebApi_Backend& waBackend_;
};

} // namespace sptf