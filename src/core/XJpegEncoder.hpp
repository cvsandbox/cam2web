/*
    cam2web - streaming camera to web

    Copyright (C) 2017, cvsandbox, cvsandbox@gmail.com

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
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
    XJpegEncoder( uint16_t quality = 85, bool fasterCompression = false );
    ~XJpegEncoder( );

    // Set/get compression quality, [0, 100]
    uint16_t Quality( ) const;
    void SetQuality( uint16_t quality );

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
