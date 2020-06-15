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

#include <string.h>
#include <new>

#include "XImage.hpp"

using namespace std;

// Returns number of bits required for pixel in certain format
uint32_t XImageBitsPerPixel( XPixelFormat format )
{
    static int sizes[]     = { 0, 8, 24, 32, 8 };
    int        formatIndex = static_cast<int>( format );

    return ( formatIndex >= ( sizeof( sizes ) / sizeof( sizes[0] ) ) ) ? 0 : sizes[formatIndex];
}

// Returns number of bytes per stride when number of bits per line is known (stride is always 32 bit aligned)
static uint32_t XImageBytesPerStride( uint32_t bitsPerLine )
{
    return ( ( bitsPerLine + 31 ) & ~31 ) >> 3;
}

// Returns number of bytes per line when number of bits per line is known (line is always 8 bit aligned)
static uint32_t XImageBytesPerLine( uint32_t bitsPerLine )
{
    return ( bitsPerLine + 7 ) >> 3;
}

// Create empty image
XImage::XImage( uint8_t* data, int32_t width, int32_t height, int32_t stride, XPixelFormat format, bool ownMemory ) :
    mData( data ), mWidth( width ), mHeight( height ), mStride( stride ), mFormat( format ), mOwnMemory( ownMemory )
{
}

// Destroy image
XImage::~XImage( )
{
    if ( ( mOwnMemory ) && ( mData != nullptr ) )
    {
        free( mData );
    }
}

// Allocate image of the specified size and format
shared_ptr<XImage> XImage::Allocate( int32_t width, int32_t height, XPixelFormat format, bool zeroInitialize )
{
    int32_t  stride = (int32_t) XImageBytesPerStride( width * XImageBitsPerPixel( format ) );
    XImage*  image  = nullptr;
    uint8_t* data   = nullptr;

    if ( zeroInitialize )
    {
        data = (uint8_t*) calloc( 1, height * stride );
    }
    else
    {
        data = (uint8_t*) malloc( height * stride );
    }

    if ( data != nullptr )
    {
        image = new (nothrow) XImage( data, width, height, stride, format, true );
    }

    return shared_ptr<XImage>( image );
}

// Create image by wrapping existing memory buffer
shared_ptr<XImage> XImage::Create( uint8_t* data, int32_t width, int32_t height, int32_t stride, XPixelFormat format )
{
    return shared_ptr<XImage>( new (nothrow) XImage( data, width, height, stride, format, false ) );
}

// Clone image - make a deep copy of it
shared_ptr<XImage> XImage::Clone( ) const
{
    shared_ptr<XImage> clone;

    if ( mData != nullptr )
    {
        clone = Allocate( mWidth, mHeight, mFormat );

        if ( clone )
        {
            if ( CopyData( clone ) != XError::Success )
            {
                clone.reset( );
            }
        }
    }

    return clone;
}

// Copy content of the image - destination image must have same width/height/format
XError XImage::CopyData( const shared_ptr<XImage>& copyTo ) const
{
    XError ret = XError::Success;

    if ( ( mData == nullptr ) || ( !copyTo ) || ( copyTo->mData == nullptr ) )
    {
        ret = XError::NullPointer;
    }
    // for JPEGs we just make sure there is enough space to copy the image data,
    // but for all uncompressed formats we check for exact match of width/height
    else if ( ( mHeight != copyTo->mHeight ) || ( mFormat != copyTo->mFormat ) )
    {
        ret = XError::ImageParametersMismatch;
    }
    else if  ( ( ( mFormat != XPixelFormat::JPEG ) && ( mWidth != copyTo->mWidth ) ) ||
               ( ( mFormat == XPixelFormat::JPEG ) && ( mStride > copyTo->mStride ) ) )
    {
        ret = XError::ImageParametersMismatch;
    }
    else
    {
        uint32_t lineSize  = XImageBytesPerLine( mWidth * XImageBitsPerPixel( mFormat ) );
        uint8_t* srcPtr    = mData;
        uint8_t* dstPtr    = copyTo->mData;
        int32_t  dstStride = copyTo->mStride;

        for ( int y = 0; y < mHeight; y++ )
        {
            memcpy( dstPtr, srcPtr, lineSize );
            srcPtr += mStride;
            dstPtr += dstStride;
        }

        if ( mFormat == XPixelFormat::JPEG )
        {
            // set correct size of destionation JPEG image
            copyTo->mWidth = mWidth;
        }
    }

    return ret;
}

// Copy content of the image into the specified one if its size/format is same or make a clone
XError XImage::CopyDataOrClone( std::shared_ptr<XImage>& copyTo ) const
{
    XError ret = XError::Success;

    if ( ( !copyTo ) ||
         ( copyTo->Height( ) != mHeight ) ||
         ( copyTo->Format( ) != mFormat ) ||
         ( ( mFormat != XPixelFormat::JPEG ) && ( copyTo->Width( ) != mWidth ) ) ||
         ( ( mFormat == XPixelFormat::JPEG ) && ( copyTo->Stride( ) < mStride ) )
       )
    {
        copyTo = Clone( );
        if ( !copyTo )
        {
            ret = XError::OutOfMemory;
        }
    }
    else
    {
        ret = CopyData( copyTo );
    }

    return ret;
}
