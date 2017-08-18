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

#include "XDeviceCapabilities.hpp"

XDeviceCapabilities::XDeviceCapabilities( ) :
    mWidth( 0 ), mHeight( 0 ), mBits( 0 ), mAvgFps( 0 ), mMaxFps( 0 ), mMinFps( 0 )
{
}

XDeviceCapabilities::XDeviceCapabilities( int width, int height, int bits, int avgFps, int maxFps, int minFps ) :
    mWidth( width ), mHeight( height ), mBits( bits ), mAvgFps( avgFps ), mMaxFps( maxFps ), mMinFps( minFps )
{
}

// Check if two capabilities are equal or not (width/height/bpp)
bool XDeviceCapabilities::operator==( const XDeviceCapabilities& rhs ) const
{
    return ( ( mWidth == rhs.mWidth ) && ( mHeight == rhs.mHeight ) && ( mBits == rhs.mBits ) && ( mMaxFps == rhs.mMaxFps ) );
}
