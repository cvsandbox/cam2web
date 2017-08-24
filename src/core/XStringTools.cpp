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

#include <algorithm>
#include <functional>
#include <cctype>

#include "XStringTools.hpp"

using namespace std;

// Trim spaces from the start of a string
string& StringLTrimg( string& s )
{
    s.erase( s.begin( ), std::find_if( s.begin( ), s.end( ),
        std::not1( std::ptr_fun<int, int>( std::isspace ) ) ) );
    return s;
}

// Trim spaces from the end of a string
string& StringRTrim( string& s )
{
    s.erase( std::find_if( s.rbegin( ), s.rend( ),
        std::not1( std::ptr_fun<int, int>( std::isspace ) ) ).base( ), s.end( ) );
    return s;
}

// Trim spaces from both ends of a string
string& StringTrim( string& s )
{
    return StringLTrimg( StringRTrim( s ) );
}

// Replace sub-string within a string
string& StringReplace( string& s, const string& lookFor, const string& replaceWith )
{
    if ( !lookFor.empty( ) )
    {
        size_t index          = 0;
        size_t lookForLen     = lookFor.length( );
        size_t replaceWithLen = replaceWith.length( );
        
        while ( index != string::npos )
        {
            index = s.find( lookFor, index );

            if ( index != string::npos )
            {
                s.replace( index, lookForLen, replaceWith );

                index += replaceWithLen;
            }
        }
    }

    return s;
}
