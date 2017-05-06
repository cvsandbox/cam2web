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

#ifndef XDEVICE_CAPABILITIES_HPP
#define XDEVICE_CAPABILITIES_HPP

// DirectShow device's video capabilities (most important ones)
class XDeviceCapabilities
{
public:
	XDeviceCapabilities( );
	XDeviceCapabilities( int width, int height, int bits, int avgFps, int maxFps );

	int Width( ) const { return mWidth; }
	int Height( ) const { return mHeight; }
	int BitCount( ) const { return mBits; }
	int AverageFrameRate( ) const { return mAvgFps; }
	int MaximumFrameRate( ) const { return mMaxFps; }

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
};

#endif // XDEVICE_CAPABILITIES_HPP
