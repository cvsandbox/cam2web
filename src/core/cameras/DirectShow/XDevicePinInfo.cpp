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
