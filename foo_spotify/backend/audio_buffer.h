#pragma once

#include <nonstd/span.hpp>

#include <array>
#include <atomic>
#include <mutex>

namespace sptf
{

// TODO: bench and replace with lock-free if needed
class AudioBuffer
{
    static constexpr size_t k_maxBufferSize = 8UL*1024*1024;
public:
#pragma pack( push )
#pragma pack( 1 )
    struct AudioChunkHeader
    {
        uint32_t sampleRate;
        uint16_t channels;
        uint32_t size;
    };
#pragma pack( pop )

private:
    static constexpr size_t k_headerSizeInU16 = sizeof( AudioChunkHeader ) / sizeof( uint16_t );

public:
    AudioBuffer() = default;
    ~AudioBuffer() = default;

    bool write( AudioChunkHeader header, const uint16_t* data )
    {
        std::lock_guard lock( posMutex_ );

        const size_t readPos = readPos_;
        const size_t writePos = writePos_;
        const size_t waterMark = waterMark_;
        const size_t writeSize = k_headerSizeInU16 + header.size;

        const auto writeData = [&]( size_t writePos ) {
            const auto curBufferPos = begin_ + writePos;
            std::copy( &header, &header + 1, reinterpret_cast<AudioChunkHeader*>( curBufferPos ) );
            std::copy( data, data + header.size, curBufferPos + k_headerSizeInU16 );
        };

        // TODO: simplify the code
        if ( writePos >= readPos ) // write leads
        {
            if ( size_ - writePos >= writeSize )
            {
                writeData( writePos );

                writePos_ = writePos + writeSize;
                waterMark_ = size_;
            }
            else // wrap-around
            {
                if ( !readPos || readPos - 1 < writeSize )
                {
                    return false;
                }

                writeData( 0 );

                writePos_ = writeSize;
                waterMark_ = writePos;
            }
        }
        else // read leads
        {
            if ( readPos - 1 - writePos < writeSize )
            {
                return false;
            }

            writeData( writePos );

            writePos_ = writePos + writeSize;
        }

        return true;
    }

    void write_end()
    {
        std::lock_guard lock( posMutex_ );
        hasEnded_ = true;
    }

    template <typename Fn>
    bool read( Fn fn )
    {
        std::lock_guard lock( posMutex_ );

        size_t readPos = readPos_;
        const size_t writePos = writePos_;
        const size_t waterMark = waterMark_;

        if (readPos == writePos)
        {
            return false;
        }

        readPos = ( readPos == waterMark ? 0 : readPos );

        const auto curBufferPos = begin_ + readPos;
        fn( *reinterpret_cast<AudioChunkHeader*>( curBufferPos ), curBufferPos + k_headerSizeInU16 );

        readPos_ = readPos + k_headerSizeInU16 + reinterpret_cast<AudioChunkHeader*>( curBufferPos )->size;

        return true;
    }

    bool has_data() const 
    {
        std::lock_guard lock( posMutex_ );

        return ( readPos_ != writePos_ );
    }

    bool has_ended() const 
    {
        std::lock_guard lock( posMutex_ );
        return hasEnded_;
    }

    void clear()
    {
        std::lock_guard lock( posMutex_ );

        readPos_ = 0;
        writePos_ = 0;
        waterMark_ = size_;
        hasEnded_ = false;
    }

private:
    std::array<uint16_t, k_maxBufferSize> buffer_;
    uint16_t* begin_ = buffer_.data();
    static constexpr size_t size_ = k_maxBufferSize;

    mutable std::mutex posMutex_;
    size_t readPos_ = 0;
    size_t writePos_ = 0;
    size_t waterMark_ = size_;

    bool hasEnded_ = false;
};

} // namespace sptf
