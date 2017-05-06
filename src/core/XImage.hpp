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

#ifndef XIMAGE_HPP
#define XIMAGE_HPP

#include <memory>

#include "XInterfaces.hpp"
#include "XError.hpp"

enum class XPixelFormat
{
    Unknown = 0,
    Grayscale8,
    RGB24,
    RGBA32,

    JPEG,
    // Enough for this project
};

enum
{
    RedIndex   = 0,
    GreenIndex = 1,
    BlueIndex  = 2
};

// Class encapsulating image data
class XImage : private Uncopyable
{
private:
    XImage( uint8_t* data, int32_t width, int32_t height, int32_t stride, XPixelFormat format, bool ownMemory );

public:
    ~XImage( );

    // Allocate image of the specified size and format
    static std::shared_ptr<XImage> Allocate( int32_t width, int32_t height, XPixelFormat format, bool zeroInitialize = false );
    // Create image by wrapping existing memory buffer
    static std::shared_ptr<XImage> Create( uint8_t* data, int32_t width, int32_t height, int32_t stride, XPixelFormat format );

    // Clone image - make a deep copy of it
    std::shared_ptr<XImage> Clone( ) const;
    // Copy content of the image - destination image must have same width/height/format
    XError CopyData( const std::shared_ptr<XImage>& copyTo ) const;
    // Copy content of the image into the specified one if its size/format is same or make a clone
    XError CopyDataOrClone( std::shared_ptr<XImage>& copyTo ) const;

    // Image properties
    int32_t Width( )       const { return mWidth;  }
    int32_t Height( )      const { return mHeight; }
    int32_t Stride( )      const { return mStride; }
    XPixelFormat Format( ) const { return mFormat; }
    // Raw data of the image
    uint8_t* Data( )       const { return mData;   }

private:
    uint8_t*     mData;
    int32_t      mWidth;
    int32_t      mHeight;
    int32_t      mStride;
    XPixelFormat mFormat;
    bool         mOwnMemory;
};

#endif // XIMAGE_HPP
