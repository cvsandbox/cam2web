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

#ifndef XJPEG_ENCODER_HPP
#define XJPEG_ENCODER_HPP

#include <stdint.h>

#include "XInterfaces.hpp"
#include "XImage.hpp"
#include "XError.hpp"

namespace Private
{
    class XJpegEncoderData;
}

class XJpegEncoder : private Uncopyable
{
public:
    XJpegEncoder( uint32_t quality = 85, bool fasterCompression = false );
    ~XJpegEncoder( );

    // Set/get compression quality, [0, 100]
    uint32_t Quality( ) const;
    void SetQuality( uint32_t quality );

    // Set/get faster compression (but less accurate) flag
    bool FasterCompression( ) const;
    void SetFasterCompression( bool faster );

    /* Compress the specified image into provided buffer

       On input, buffer size must be set to the size of provided buffer.
       On output, it is set to the size of encoded JPEG image. If provided
       buffer is too small, it will be re-allocated (realloc).
    */
    XError EncodeToMemory( const std::shared_ptr<const XImage>& image, uint8_t** buffer, uint32_t* bufferSize );

private:
    Private::XJpegEncoderData* mData;
};

#endif // XJPEG_ENCODER_HPP
