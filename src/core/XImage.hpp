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
