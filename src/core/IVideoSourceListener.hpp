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

#ifndef IVIDEO_SOURCE_LISTENER_HPP
#define IVIDEO_SOURCE_LISTENER_HPP

#include "XImage.hpp"
#include <string>

class IVideoSourceListener
{
public:
    virtual ~IVideoSourceListener( ) { }

    // New video frame notification
    virtual void OnNewImage( const std::shared_ptr<const XImage>& image ) = 0;

    // Video source error notification
    virtual void OnError( const std::string& errorMessage, bool fatal ) = 0;
};

#endif // IVIDEO_SOURCE_LISTENER_HPP
