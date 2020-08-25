#pragma once

#include <nonstd/span.hpp>

#include <array>
#include <atomic>
#include <mutex>

namespace sptf
{

class AbortManager;

// TODO: bench and replace with lock-free if needed
class AudioBuffer
{
    static constexpr size_t k_maxBufferSize = 8UL * 1024 * 1024;

public:
#pragma pack( push )
#pragma pack( 1 )
    struct AudioChunkHeader
    {
        uint32_t sampleRate;
        uint16_t channels;
        uint32_t size;
        uint16_t eof;
    };
#pragma pack( pop )

private:
    static constexpr size_t k_headerSizeInU16 = sizeof( AudioChunkHeader ) / sizeof( uint16_t );

public:
    AudioBuffer( AbortManager& abortManager );
    ~AudioBuffer() = default;

    bool write( AudioChunkHeader header, const uint16_t* data );
    void write_end();

    template <typename Fn>
    bool read( Fn fn );

    bool has_data() const;
    bool wait_for_data( abort_callback& abort );

    void clear();

private:
    bool has_data_no_lock() const;

private:
    AbortManager& abortManager_;

    std::array<uint16_t, k_maxBufferSize> buffer_;
    uint16_t* begin_ = buffer_.data();
    static constexpr size_t size_ = k_maxBufferSize;

    mutable std::mutex posMutex_;
    std::condition_variable dataCv_;

    size_t readPos_ = 0;
    size_t writePos_ = 0;
    size_t waterMark_ = size_;
};

template <typename Fn>
bool sptf::AudioBuffer::read( Fn fn )
{
    std::lock_guard lock( posMutex_ );

    size_t readPos = readPos_;
    const size_t writePos = writePos_;
    const size_t waterMark = waterMark_;

    if ( readPos == writePos )
    {
        return false;
    }

    readPos = ( readPos == waterMark ? 0 : readPos );

    const auto curBufferPos = begin_ + readPos;
    fn( *reinterpret_cast<AudioChunkHeader*>( curBufferPos ), curBufferPos + k_headerSizeInU16 );

    readPos_ = readPos + k_headerSizeInU16 + reinterpret_cast<AudioChunkHeader*>( curBufferPos )->size;

    return true;
}

} // namespace sptf
