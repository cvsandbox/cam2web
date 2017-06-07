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

#include <list>

#include "AppConfig.hpp"

using namespace std;

// Macro to make sure a value is in certain range
#define XINRANGE(a, min, max) (((a)<(max))?(((a)>(min))?(a):(min)):(max))

#define PROP_JPEG_QUALITY   "jpegQuality"
#define PROP_MJPEG_RATE     "mjpegRate"
#define PROP_HTTP_PORT      "httpPort"
#define PROP_AUTH_DOMAIN    "authDomain"

// List of supported properties
const static list<string> SupportedProperties = { PROP_JPEG_QUALITY, PROP_MJPEG_RATE, PROP_HTTP_PORT, PROP_AUTH_DOMAIN };

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
    XError  ret = XError::Success;
    int32_t intValue;
    int     scanned = sscanf( value.c_str( ), "%d", &intValue );

    if ( propertyName == PROP_JPEG_QUALITY )
    {
        if ( scanned != 1 )
        {
            ret = XError::InvalidPropertyValue;
        }
        else
        {
            jpegQuality = static_cast<uint16_t>( intValue );
        }
    }
    else if ( propertyName == PROP_MJPEG_RATE )
    {
        if ( scanned != 1 )
        {
            ret = XError::InvalidPropertyValue;
        }
        else
        {
            mjpegFrameRate = static_cast<uint16_t>( intValue );
        }
    }
    else if ( propertyName == PROP_HTTP_PORT )
    {
        if ( scanned != 1 )
        {
            ret = XError::InvalidPropertyValue;
        }
        else
        {
            httpPort = static_cast<uint16_t>( intValue );
        }
    }
    else if ( propertyName == PROP_AUTH_DOMAIN )
    {
        authDomain = value;
    }
    else
    {
        ret = XError::UnknownProperty;
    }

    return ret;
}

// Get property of the object
XError AppConfig::GetProperty( const string& propertyName, string& value ) const
{
    XError  ret         = XError::Success;
    int32_t intValue    = 0;
    bool    numericProp = false;

    if ( propertyName == PROP_JPEG_QUALITY )
    {
        intValue    = jpegQuality;
        numericProp = true;
    }
    else if ( propertyName == PROP_MJPEG_RATE )
    {
        intValue    = mjpegFrameRate;
        numericProp = true;
    }
    else if ( propertyName == PROP_HTTP_PORT )
    {
        intValue    = httpPort;
        numericProp = true;
    }
    else if ( propertyName == PROP_AUTH_DOMAIN )
    {
        value = authDomain;
    }
    else
    {
        ret = XError::UnknownProperty;
    }

    if ( ( ret ) && ( numericProp ) )
    {
        char buffer[32];

        sprintf( buffer, "%d", intValue );
        value = buffer;
    }

    return ret;
}

// Get all properties of the object
map<string, string> AppConfig::GetAllProperties( ) const
{
    map<string, string> properties;
    string              value;

    for ( auto property : SupportedProperties )
    {
        if ( GetProperty( property, value ) )
        {
            properties.insert( pair<string, string>( property, value ) );
        }
    }

    return properties;
}
