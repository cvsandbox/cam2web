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

#ifndef XDEVICE_CAPABILITIES_HPP
#define XDEVICE_CAPABILITIES_HPP

// DirectShow device's video capabilities (most important ones)
class XDeviceCapabilities
{
public:
    XDeviceCapabilities( );
    XDeviceCapabilities( int width, int height, int bits, int avgFps, int maxFps, int minFps );

    int Width( ) const { return mWidth; }
    int Height( ) const { return mHeight; }
    int BitCount( ) const { return mBits; }
    int AverageFrameRate( ) const { return mAvgFps; }
    int MaximumFrameRate( ) const { return mMaxFps; }
    int MinimumFrameRate( ) const { return mMinFps; }

    // Check if two capabilities are equal or not (width/height/bpp)
    bool operator==( const XDeviceCapabilities& rhs ) const;
    bool operator!=( const XDeviceCapabilities& rhs  ) const
    {
        return ( !( (*this) == rhs ) ) ;
    }

private:
    int mWidth;
    int mHeight;
    int mBits;
    int mAvgFps;
    int mMaxFps;
    int mMinFps;
};

#endif // XDEVICE_CAPABILITIES_HPP
