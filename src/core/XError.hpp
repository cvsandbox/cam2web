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

#ifndef XERROR_HPP
#define XERROR_HPP

#include <string>

class XError
{
public:

    enum ErrorCode
    {
        Success = 0,

        Failed,                     // Generic failure
        NullPointer,                // Input parameter is a null pointer
        OutOfMemory,                // Out of memory
        DeivceNotReady,             // Device (whatever it might be) is not ready for the requested action
        ConfigurationNotSupported,  // Configuration is not supported by device/object/whoever
        UnknownProperty,            // Specified property is not known
        InvalidPropertyValue,       // Specified property value is not valid
        ReadOnlyProperty,           // Specified property is read only
        UnsupportedPixelFormat,     // Pixel format (of an image) is not supported
        ImageParametersMismatch,    // Parameters of images (width/height/format) don't match
        FailedImageEncoding         // Failed image encoding
    };

public:
    XError( ErrorCode code = Success ) : mCode( code ) { }

    // Get the error code
    operator ErrorCode( ) const { return mCode; }
    // Check if error code is Success
    operator bool( ) const { return ( mCode == Success ); }
    // Get error code as integer
    int Code( ) const { return static_cast<int>( mCode ); }

    std::string ToString( ) const;

private:
    ErrorCode mCode;
};

#endif // XERROR_HPP
