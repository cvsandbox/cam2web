/*
    cam2web - streaming camera to web

    Copyright (C) 2017-2019, cvsandbox, cvsandbox@gmail.com

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

// Definition of ARGB color type
typedef union
{
    uint32_t argb;
    struct
    {
        uint8_t b;
        uint8_t g;
        uint8_t r;
        uint8_t a;
    }
    components;
}
xargb;

// A very close approximation of the famous BT709 Grayscale coefficients: (0.2125, 0.7154, 0.0721)
// Pre-multiplied by 0x10000, so integer grayscaling could be done
#define GRAY_COEF_RED   (0x3666)
#define GRAY_COEF_GREEN (0xB724)
#define GRAY_COEF_BLUE  (0x1276)

// A macro to get gray value (intensity) out of RGB values
#define RGB_TO_GRAY(r, g, b) ((uint32_t) ( GRAY_COEF_RED * (r) + GRAY_COEF_GREEN * (g) + GRAY_COEF_BLUE * (b) ) >> 16 )


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
