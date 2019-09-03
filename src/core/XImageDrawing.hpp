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

#pragma once
#ifndef XIMAGE_DRAWING_HPP
#define XIMAGE_DRAWING_HPP

#include <string>
#include "XImage.hpp"
#include "XError.hpp"

class XImageDrawing
{
public:
    XImageDrawing( ) = delete;

public:

    // Draw horizontal line on the specified image
    static XError HLine( const std::shared_ptr<const XImage>& image, int32_t x1, int32_t x2, int32_t y, xargb color );

    // Draw vertical line on the specified image
    static XError VLine( const std::shared_ptr<const XImage>& image, int32_t y1, int32_t y2, int32_t x, xargb color );

    // Draw rectangle on the specified image with the specfied color (all coordinates are inclusive)
    static XError Rectangle( const std::shared_ptr<const XImage>& image, int32_t x1, int32_t y1, int32_t x2, int32_t y2, xargb color );

    // Draw ASCII text on the image at the specified location
    static XError PutText( const std::shared_ptr<const XImage>& image, const std::string& text, int32_t x, int32_t y, xargb color, xargb background, bool addBorder = true );
};

#endif // XIMAGE_DRAWING_HPP
