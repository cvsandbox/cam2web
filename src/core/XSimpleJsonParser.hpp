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

#ifndef XSIMPLE_JSON_PARSER_HPP
#define XSIMPLE_JSON_PARSER_HPP

#include <string>
#include <map>

/* ================================================================= */
/* The function implements a *VERY* simple JSON parser. It does NOT  */
/* support nested objects/arrays - those are returned as strings.    */
/* In fact all values are returned as strings in a map. It is up to  */
/* caller to know type of a particular value and perform further     */
/* checking/parsing.                                                 */
/* ================================================================= */

bool XSimpleJsonParser( const std::string& jsonStr, std::map<std::string, std::string>& values );

#endif // XSIMPLE_JSON_PARSER_HPP
