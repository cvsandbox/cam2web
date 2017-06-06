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

#ifndef APP_CONFIG_HPP
#define APP_CONFIG_HPP

#include <IObjectConfigurator.hpp>
#include <stdint.h>

class AppConfig : public IObjectConfigurator
{
public:
    AppConfig( );

    // Get/Set JPEG quality
    uint16_t JpegQuality( ) const;
    void SetJpegQuality( uint16_t quality );

    // Get/Set MJPEG frame rate
    uint16_t MjpegFrameRate( ) const;
    void SetMjpegFrameRate( uint16_t frameRate );

    // Get/Set HTTP port to listen
    uint16_t HttpPort( ) const;
    void SetHttpPort( uint16_t port );

    // Get/Set HTTP digest auth domain
    std::string AuthDomain( ) const;
    void SetAuthDomain( const std::string& authDomain );

public: // IObjectConfigurator interface

    virtual XError SetProperty( const std::string& propertyName, const std::string& value );
    virtual XError GetProperty( const std::string& propertyName, std::string& value ) const;

    virtual std::map<std::string, std::string> GetAllProperties( ) const;

private:
    uint16_t    jpegQuality;
    uint16_t    mjpegFrameRate;
    uint16_t    httpPort;
    std::string authDomain;
};

#endif // APP_CONFIG_HPP
