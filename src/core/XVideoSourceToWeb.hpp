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

#ifndef XVIDEO_SOURCE_TO_WEB_HPP
#define XVIDEO_SOURCE_TO_WEB_HPP

#include <string>
#include <memory>

#include "XInterfaces.hpp"
#include "IVideoSourceListener.hpp"
#include "XWebServer.hpp"

namespace Private
{
    class XVideoSourceToWebData;
}

class XVideoSourceToWeb : private Uncopyable
{
public:
    XVideoSourceToWeb( uint16_t jpegQuality = 85 );
    ~XVideoSourceToWeb( );
    
    // Get video source listener, which could be fed to some video source
    IVideoSourceListener* VideoSourceListener( ) const;

    // Create web request handler to provide camera images as JPEGs
    std::shared_ptr<IWebRequestHandler> CreateJpegHandler( const std::string& uri ) const;

    // Create web request handler to provide camera images as MJPEG stream
    std::shared_ptr<IWebRequestHandler> CreateMjpegHandler( const std::string& uri, uint32_t frameRate ) const;

    // Get/Set JPEG quality (valid only if camera provides uncompressed images)
    uint16_t JpegQuality( ) const;
    void SetJpegQuality( uint16_t quality );

private:
    Private::XVideoSourceToWebData* mData;
};

#endif // XVIDEO_SOURCE_TO_WEB_HPP
