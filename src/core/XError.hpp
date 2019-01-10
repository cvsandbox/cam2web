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
        IOError,                    // I/O error
        DeivceNotReady,             // Device (whatever it might be) is not ready for the requested action
        ConfigurationNotSupported,  // Configuration is not supported by device/object/whoever
        UnknownProperty,            // Specified property is not known
        UnsupportedProperty,        // Specified property is not supported by device/object/whoever
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
