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

#include "XError.hpp"

static const char* ErrorMessages[] =
{
    "Success",

    "Generic failure",
    "Input parameter is a null pointer",
    "Out of memory",
    "I/O error",
    "Device is not ready",
    "Configuration is not supported",
    "Property is not known",
    "Property is not supported",
    "Property value is not valid",
    "Property is read only",
    "Pixel format is not supported",
    "Parameters of images don't match",
    "Failed image encoding"
};

std::string XError::ToString( ) const
{
    std::string ret;

    if ( ( mCode >= 0 ) && ( mCode < sizeof( ErrorMessages ) / sizeof( ErrorMessages[0] ) ) )
    {
        ret = ErrorMessages[mCode];
    }
    else
    {
        char buffer[64];

        sprintf( buffer, "Unknown error code: %d", mCode );
        ret = buffer;
    }

    return ret;
}
