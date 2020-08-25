#include <stdafx.h>

#include "audio_buffer.h"

#include <utils/abort_manager.h>

namespace sptf
{

AudioBuffer::AudioBuffer( AbortManager& abortManager )
    : abortManager_( abortManager )
{
}

bool AudioBuffer::write( AudioChunkHeader header, const uint16_t* data )
{
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
    }

    dataCv_.notify_all();
    return true;
}

void AudioBuffer::write_end()
{
    uint16_t dummy{};
    write( AudioChunkHeader{ 0, 0, 0, 1 }, &dummy );
}

bool AudioBuffer::has_data() const
{
    std::lock_guard lock( posMutex_ );

    return has_data_no_lock();
}

bool AudioBuffer::wait_for_data( abort_callback& abort )
{
    const auto abortableScope = abortManager_.GetAbortableScope( [&] { dataCv_.notify_all(); }, abort );

    std::unique_lock lock( posMutex_ );
    dataCv_.wait( lock, [&] {
        return ( has_data_no_lock() || abort.is_aborting() );
    } );
    return has_data_no_lock();
}

void AudioBuffer::clear()
{
    {
        std::lock_guard lock( posMutex_ );

        readPos_ = 0;
        writePos_ = 0;
        waterMark_ = size_;
    }
    dataCv_.notify_all();
}

bool AudioBuffer::has_data_no_lock() const
{
    return ( readPos_ != writePos_ );
}

} // namespace sptf
