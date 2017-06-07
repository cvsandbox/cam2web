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

// Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN

#include <sdkddkver.h>
#include <windows.h>

#include "Tools.hpp"

using namespace std;

// Convert specfied UTF8 string to wide character string
wstring Utf8to16( const string& utf8string )
{
    wstring ret;

    int required = MultiByteToWideChar( CP_UTF8, 0, utf8string.c_str( ), -1, nullptr, 0 );

    if ( required > 0 )
    {
        wchar_t* utf16string = new wchar_t[required];

        if ( MultiByteToWideChar( CP_UTF8, 0, utf8string.c_str( ), -1, utf16string, required ) > 0 )
        {
            ret = wstring( utf16string );
        }

        delete[] utf16string;
    }

    return ret;
}

// Convert specfied wide character string to UTF8 string
string Utf16to8( const wstring& utf16string )
{
    string ret;

    int required = WideCharToMultiByte( CP_UTF8, 0, utf16string.c_str( ), -1, nullptr, 0, nullptr, nullptr );

    if ( required > 0 )
    {
        char* utf8string = new char[required];

        if ( WideCharToMultiByte( CP_UTF8, 0, utf16string.c_str( ), -1, utf8string, required, nullptr, nullptr ) > 0 )
        {
            ret = string( utf8string );
        }

        delete[] utf8string;
    }

    return ret;
}
