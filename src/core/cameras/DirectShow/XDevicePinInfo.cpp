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

#include "XDevicePinInfo.hpp"

using namespace std;

XDevicePinInfo::XDevicePinInfo( ) :
    mIndex( 0 ), mType( PinType::Unknown ), mIsInput( false )
{
}

XDevicePinInfo::XDevicePinInfo( int index, PinType pinType, bool isInput ) :
    mIndex( index ), mType( pinType ), mIsInput( isInput )
{
}

bool XDevicePinInfo::operator==( const XDevicePinInfo& rhs ) const
{
    return ( ( mIndex == rhs.mIndex ) && ( mType == rhs.mType ) && ( mIsInput == rhs.mIsInput )  );
}

string XDevicePinInfo::TypeToString( ) const
{
    static const char*  strTypes[] = 
    {
        "Unknown",
        "Tuner", "Composite", "SVideo", "RGB", "YRYBY", "SerialDigital",
        "Parallel Digital", "SCSI", "AUX", "1394", "USB"
        "Decoder", "Encoder", "SCART", "Black"
    };

    return string( ( ( mType >= PinType::Unknown ) && ( mType <= PinType::VideoBlack ) ) ? strTypes[static_cast<int>( mType )] : strTypes[0] );
}
