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

#ifndef XDEVICE_PIN_INFO_HPP
#define XDEVICE_PIN_INFO_HPP

#include <string>

// Types of crossbar's video pins
enum class PinType
{
    Unknown = 0,

    VideoTuner = 1,
    VideoComposite,
    VideoSVideo,
    VideoRGB,
    VideoYRYBY,
    VideoSerialDigital,
    VideoParallelDigital,
    VideoSCSI,
    VideoAUX,
    Video1394,
    VideoUSB,
    VideoDecoder,
    VideoEncoder,
    VideoSCART,
    VideoBlack
};

// DirectShow crossbar's pin info
class XDevicePinInfo
{
public:
    XDevicePinInfo( );
    XDevicePinInfo( int index, PinType pinType, bool isInput = true );

    int Index( ) const { return mIndex; }
    PinType Type( ) const { return mType; }
    std::string TypeToString( ) const;
    bool IsInput( ) const { return mIsInput; }

    // Check if two pin infos are equal or not
    bool operator==( const XDevicePinInfo& rhs ) const;
    bool operator!=( const XDevicePinInfo& rhs  ) const
    {
        return ( !( (*this) == rhs ) ) ;
    }

private:
    int     mIndex;
    PinType mType;
    bool    mIsInput;
};

#endif // XDEVICE_PIN_INFO_HPP
