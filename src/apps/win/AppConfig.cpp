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

#include "AppConfig.hpp"

using namespace std;

// Macro to make sure a value is in certain range
#define XINRANGE(a, min, max) (((a)<(max))?(((a)>(min))?(a):(min)):(max))

AppConfig::AppConfig( ) :
    jpegQuality( 85 ),
    mjpegFrameRate( 30 ),
    httpPort( 8000 ),
    authDomain( "cam2web" )
{

}

// Get/Set JPEG quality
uint16_t AppConfig::JpegQuality( ) const
{
    return jpegQuality;
}
void AppConfig::SetJpegQuality( uint16_t quality )
{
    jpegQuality = XINRANGE( quality, 1, 100 );
}

// Get/Set MJPEG frame rate
uint16_t AppConfig::MjpegFrameRate( ) const
{
    return mjpegFrameRate;
}
void AppConfig::SetMjpegFrameRate( uint16_t frameRate )
{
    mjpegFrameRate = XINRANGE( frameRate, 1, 30 );
}

// Get Set HTTP port to listen
uint16_t AppConfig::HttpPort( ) const
{
    return httpPort;
}
void AppConfig::SetHttpPort( uint16_t port )
{
    httpPort = XINRANGE( port, 1, 65535 );
}

// Get/Set HTTP digest auth domain
string AppConfig::AuthDomain( ) const
{
    return authDomain;
}
void AppConfig::SetAuthDomain( const string& domain )
{
    authDomain = domain;
}

// Set property of the object
XError AppConfig::SetProperty( const string& propertyName, const string& value )
{
    XError ret = XError::Success;

    return ret;
}

// Get property of the object
XError AppConfig::GetProperty( const string& propertyName, string& value ) const
{
    XError ret = XError::Success;

    return ret;

}

// Get all properties of the object
map<string, string> AppConfig::GetAllProperties( ) const
{
    map<string, string> properties;

    return properties;
}
