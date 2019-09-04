/*
    cam2web - streaming camera to web

    Copyright (C) 2017-2019, cvsandbox, cvsandbox@gmail.com

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

#ifndef TOOLS_HPP
#define TOOLS_HPP

#include <string>
#include <stdint.h>
#include <XImage.hpp>

// Convert specfied UTF8 string to wide character string
std::wstring Utf8to16( const std::string& utf8string );
// Convert specfied wide character string to UTF8 string
std::string Utf16to8( const std::wstring& utf16string );

// Calculate MD5 hash string for the given buffer
std::string GetMd5Hash( const uint8_t* buffer, int bufferLength );

// Get local IP address as string (if a single valid IP found). Returns empty string if fails to resolve.
std::string GetLocalIpAddress( );

// Convert xargb to string
std::string XargbToString( xargb color );
// Parse xargb from string
bool StringToXargb( const std::string& str, xargb* color );

#endif // UI_TOOLS_HPP
