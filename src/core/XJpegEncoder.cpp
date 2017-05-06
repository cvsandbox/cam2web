/*
    cam2web - streaming camera to web

    BSD 2-Clause License

    Copyright (c) 2017, cvsandbox, cvsandbox@gmail.com
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "XJpegEncoder.hpp"

#include <stdio.h>
#include <jpeglib.h>

using namespace std;

namespace Private
{
    class JpegException : public exception
    {
    public:
        virtual const char* what( ) const throw( )
        {
            return "JPEG coding failure";
        }
    };

    static void my_error_exit( j_common_ptr /* cinfo */ )
    {
        throw JpegException( );
    }

    static void my_output_message( j_common_ptr /* cinfo */ )
    {
        // do nothing - kill the message
    }

    class XJpegEncoderData
    {
    public:
        uint32_t                    Quality;
        bool                        FasterCompression;
    private:
        struct jpeg_compress_struct cinfo;
        struct jpeg_error_mgr       jerr;

    public:
        XJpegEncoderData( uint32_t quality, bool fasterCompression) :
            Quality( quality ), FasterCompression( fasterCompression  )
        {
            if ( Quality > 100 )
            {
                Quality = 100;
            }

            // allocate and initialize JPEG compression object
            cinfo.err           = jpeg_std_error( &jerr );
            jerr.error_exit     = my_error_exit;
            jerr.output_message = my_output_message;

            jpeg_create_compress( &cinfo );
        }

        ~XJpegEncoderData( )
        {
            jpeg_destroy_compress( &cinfo );
        }

        XError EncodeToMemory( const shared_ptr<const XImage>& image, uint8_t** buffer, uint32_t* bufferSize );
    };
}

XJpegEncoder::XJpegEncoder( uint32_t quality, bool fasterCompression ) :
    mData( new Private::XJpegEncoderData( quality, fasterCompression ) )
{

}

XJpegEncoder::~XJpegEncoder( )
{
    delete mData;
}

// Set/get compression quality, [0, 100]
uint32_t XJpegEncoder::Quality( ) const
{
    return mData->Quality;
}
void XJpegEncoder::SetQuality( uint32_t quality )
{
    mData->Quality = quality;
}

// Set/get faster compression (but less accurate) flag
bool XJpegEncoder::FasterCompression( ) const
{
    return mData->FasterCompression;
}
void XJpegEncoder::SetFasterCompression( bool faster )
{
    mData->FasterCompression = faster;
}

// Compress the specified image into provided buffer
XError XJpegEncoder::EncodeToMemory( const shared_ptr<const XImage>& image, uint8_t** buffer, uint32_t* bufferSize )
{
    return mData->EncodeToMemory( image, buffer, bufferSize );
}

namespace Private
{

XError XJpegEncoderData::EncodeToMemory( const shared_ptr<const XImage>& image, uint8_t** buffer, uint32_t* bufferSize )
{
    JSAMPROW    row_pointer[1];
    XError      ret = XError::Success;

    if ( ( !image ) || ( image->Data( ) == nullptr ) || ( buffer == nullptr ) || ( *buffer == nullptr ) || ( bufferSize == nullptr ) )
    {
        ret = XError::NullPointer;
    }
    else if ( ( image->Format( ) != XPixelFormat::RGB24 ) && ( image->Format( ) != XPixelFormat::Grayscale8 ) )
    {
        ret = XError::UnsupportedPixelFormat;
    }
    else
    {
        try
        {
            // 1 - specify data destination
            unsigned long mem_buffer_size = *bufferSize;
            jpeg_mem_dest( &cinfo, buffer, &mem_buffer_size );

            // 2 - set parameters for compression
            cinfo.image_width  = image->Width( );
            cinfo.image_height = image->Height( );

            if ( image->Format( ) == XPixelFormat::RGB24 )
            {
                cinfo.input_components = 3;
                cinfo.in_color_space   = JCS_RGB;
            }
            else
            {
                cinfo.input_components = 1;
                cinfo.in_color_space   = JCS_GRAYSCALE;
            }

            // set default compression parameters
            jpeg_set_defaults( &cinfo );
            // set quality
            jpeg_set_quality( &cinfo, (int) Quality, TRUE /* limit to baseline-JPEG values */ );

            // use faster, but less accurate compressions
            cinfo.dct_method = ( FasterCompression ) ? JDCT_FASTEST : JDCT_DEFAULT;

            // 3 - start compressor
            jpeg_start_compress( &cinfo, TRUE );

            // 4 - do compression
            while ( cinfo.next_scanline < cinfo.image_height )
            {
                row_pointer[0] = image->Data( ) + image->Stride( ) * cinfo.next_scanline;

                jpeg_write_scanlines( &cinfo, row_pointer, 1 );
            }

            // 5 - finish compression
            jpeg_finish_compress( &cinfo );

            *bufferSize = (uint32_t) mem_buffer_size;
        }
        catch ( const JpegException& )
        {
            ret = XError::FailedImageEncoding;
        }
    }

    return ret;
}

} // namespace Private
