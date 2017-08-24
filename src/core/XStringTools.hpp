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

#ifndef XSTRING_TOOLS_HPP
#define XSTRING_TOOLS_HPP

#include <string>

// Trim spaces from the start of a string
std::string& StringLTrimg( std::string& s );
// Trim spaces from the end of a string
std::string& StringRTrim( std::string& s );
// Trim spaces from both ends of a string
std::string& StringTrim( std::string& s );

// Replace sub-string within a string
std::string& StringReplace( std::string& s, const std::string& lookFor, const std::string& replaceWith );

#endif // XSTRING_TOOLS_HPP
