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
