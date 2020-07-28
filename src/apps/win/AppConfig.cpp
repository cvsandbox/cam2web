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

#include <map>

#include "AppConfig.hpp"
#include "Tools.hpp"

using namespace std;

// Macro to make sure a value is in certain range
#define XINRANGE(a, min, max) (((a)<(max))?(((a)>(min))?(a):(min)):(max))

#define PROP_JPEG_QUALITY       "jpegQuality"
#define PROP_MJPEG_RATE         "mjpegRate"
#define PROP_HTTP_PORT          "httpPort"
#define PROP_VIEWERS            "viewers"
#define PROP_CONFIGURATORS      "configurators"
#define PROP_AUTH_DOMAIN        "authDomain"
#define PROP_AUTH_METHOD        "authMethod"
#define PROP_CUSTOM_WEB         "customWeb"
#define PROP_CAMERA_MONIKER     "cameraMoniker"
#define PROP_CAMERA_TITLE       "cameraTitle"
#define PROP_DEVICE_PREFERENCE  "devicePreference"
#define PROP_TIMESTAMP_OVR      "timestampOverlay"
#define PROP_TITLE_OVR          "cameraTitleOverlay"
#define PROP_OVR_TEXT_COLOR     "overlayTextColor"
#define PROP_OVR_BG_COLOR       "overlayBgColor"
#define PROP_CAMERA_WIDTH       "cameraWidth"
#define PROP_CAMERA_HEIGHT      "cameraHeight"
#define PROP_CAMERA_BPP         "cameraBpp"
#define PROP_CAMERA_FPS         "cameraFps"
#define PROP_REQUESTED_FPS      "requestedFps"
#define PROP_MIN_TO_TRAY        "minToTray"
#define PROP_WINDOW_ICON        "windowIcon"
#define PROP_MAIN_WIN_X         "mainWinX"
#define PROP_MAIN_WIN_Y         "mainWinY"

#define TYPE_INT (0)
#define TYPE_STR (1)

// List of supported properties and their type
const static map<string, int> SupportedProperties =
{
    { PROP_JPEG_QUALITY,        TYPE_INT },
    { PROP_MJPEG_RATE,          TYPE_INT },
    { PROP_HTTP_PORT,           TYPE_INT },
    { PROP_VIEWERS,             TYPE_INT },
    { PROP_CONFIGURATORS,       TYPE_INT },
    { PROP_AUTH_DOMAIN,         TYPE_STR },
    { PROP_AUTH_METHOD,         TYPE_STR },
    { PROP_CUSTOM_WEB,          TYPE_STR },
    { PROP_CAMERA_MONIKER,      TYPE_STR },
    { PROP_CAMERA_TITLE,        TYPE_STR },
    { PROP_TIMESTAMP_OVR,       TYPE_INT },
    { PROP_TITLE_OVR,           TYPE_INT },
    { PROP_OVR_TEXT_COLOR,      TYPE_STR },
    { PROP_OVR_BG_COLOR,        TYPE_STR },
    { PROP_CAMERA_WIDTH,        TYPE_INT },
    { PROP_CAMERA_HEIGHT,       TYPE_INT },
    { PROP_CAMERA_BPP,          TYPE_INT },
    { PROP_CAMERA_FPS,          TYPE_INT },
    { PROP_REQUESTED_FPS,       TYPE_INT },
    { PROP_MIN_TO_TRAY,         TYPE_INT },
    { PROP_WINDOW_ICON,         TYPE_INT },
    { PROP_MAIN_WIN_X,          TYPE_INT },
    { PROP_MAIN_WIN_Y,          TYPE_INT },
    { PROP_DEVICE_PREFERENCE,   TYPE_STR }
};

AppConfig::AppConfig( ) :
    jpegQuality( 85 ),
    mjpegFrameRate( 30 ),
    httpPort( 8000 ),
    viewersGroup( 0 ),
    configuratorsGroup( 0 ),
    authDomain( "cam2web" ),
    authMethod( "digest" ),
    customWebContent( ),
    cameraMoniker( ),
    cameraTitle( ),
    addTimestampOverlay( false ),
    addCameraTitleOverlay( false ),
    overlayTextColor( { 0xFF000000 } ),
    overlayBackgroundColor( { 0xFFFFFFFF } ),
    cameraWidth( 0 ),
    cameraHeight( 0 ),
    cameraBpp( 0 ),
    cameraFps( 0 ),
    requestedFps( 0 ),
    minimizeToSystemTray( false ),
    windowIcon( 0 ),
    mainWindowX( 0x0FFFFFFF ), mainWindowY( 0x0FFFFFFF ),
    usersFileName( "users.txt" ),
    devicePreference( )
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

// Get/Set viewers group ID
uint16_t AppConfig::ViewersGroup( )
{
    return viewersGroup;
}
void AppConfig::SetViewersGroup( uint16_t groupId )
{
    viewersGroup = groupId;
}

// Get/Set configurators group ID
uint16_t AppConfig::ConfiguratorsGroup( )
{
    return configuratorsGroup;
}
void AppConfig::SetConfiguratorsGroup( uint16_t groupId )
{
    configuratorsGroup = groupId;
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

// Get/Set HTTP authentication method
string AppConfig::AuthenticationMethod( ) const
{
    return authMethod;
}
void AppConfig::SetAuthenticationMethod( const string& method )
{
    authMethod = method;
}

// Get/Set path to the folder with custom web content
string AppConfig::CustomWebContent( ) const
{
    return customWebContent;
}
void AppConfig::SetCustomWebContent( const string& path )
{
    customWebContent = path;
}

// Get/Set camera moniker string
string AppConfig::CameraMoniker( ) const
{
    return cameraMoniker;
}
void AppConfig::SetCameraMoniker( const string& moniker )
{
    cameraMoniker = moniker;
}

// Get/Set camera title
string AppConfig::CameraTitle( ) const
{
    return cameraTitle;
}
void AppConfig::SetCameraTitle( const string& title )
{
    cameraTitle = title;
}

// Get/Set if timestamp should be overlayed on camera images
bool AppConfig::TimestampOverlay( ) const
{
    return addTimestampOverlay;
}
void AppConfig::SetTimestampOverlay( bool enabled )
{
    addTimestampOverlay = enabled;
}

// Get/Set if camera's title should be overlayed on its images
bool AppConfig::CameraTitleOverlay( ) const
{
    return addCameraTitleOverlay;
}
void AppConfig::SetCameraTitleOverlay( bool enabled )
{
    addCameraTitleOverlay = enabled;
}

// Get/Set overlay text color
xargb AppConfig::OverlayTextColor( ) const
{
    return overlayTextColor;
}
void AppConfig::SetOverlayTextColor( xargb color )
{
    overlayTextColor = color;
}

// Get/Set overlay background color
xargb AppConfig::OverlayBackgroundColor( ) const
{
    return overlayBackgroundColor;
}
void AppConfig::SetOverlayBackgroundColor( xargb color )
{
    overlayBackgroundColor = color;
}

// Get/Set last used video resolution
void AppConfig::GetLastVideoResolution( uint16_t* width, uint16_t* height, uint16_t* bpp, uint16_t* fps ) const
{
    if ( width  != nullptr ) *width  = cameraWidth;
    if ( height != nullptr ) *height = cameraHeight;
    if ( bpp    != nullptr ) *bpp    = cameraBpp;
    if ( fps    != nullptr ) *fps    = cameraFps;
}
void AppConfig::SetLastVideoResolution( uint16_t width, uint16_t height, uint16_t bpp, uint16_t fps )
{
    cameraWidth  = width;
    cameraHeight = height;
    cameraBpp    = bpp;
    cameraFps    = fps;
}

// Get/Set last requested frame rate
uint16_t AppConfig::LastRequestedFrameRate( ) const
{
    return requestedFps;
}
void AppConfig::SetLastRequestedFrameRate( uint16_t fps )
{
    requestedFps = fps;
}

// Get/Set the flag inidicating that application should minimize to system tray
bool AppConfig::MinimizeToSystemTray( ) const
{
    return minimizeToSystemTray;
}
void AppConfig::SetMinimizeToSystemTray( bool enabled )
{
    minimizeToSystemTray = enabled;
}

// Get/Set index of the window/tray icon to use
uint16_t AppConfig::WindowIconIndex( ) const
{
    return windowIcon;
}
void AppConfig::SetWindowIconIndex( uint16_t index )
{
    windowIcon = index;
}

// Get/Set main window's position
int32_t AppConfig::MainWindowX( ) const
{
    return mainWindowX;
}
int32_t AppConfig::MainWindowY( ) const
{
    return mainWindowY;
}
void AppConfig::SetMainWindowXY( int32_t x, int32_t y )
{
    mainWindowX = x;
    mainWindowY = y;
}

// Get/Set file name to store users' list in
string AppConfig::UsersFileName( ) const
{
    return usersFileName;
}
void AppConfig::SetUsersFileName( const string& fileName )
{
    usersFileName = fileName;
}

// Get user camaera device preference
string AppConfig::DevicePreference() const
{
    return devicePreference;
}

// Set property of the object
XError AppConfig::SetProperty( const string& propertyName, const string& value )
{
    XError                           ret = XError::Success;
    map<string, int>::const_iterator itProperty = SupportedProperties.find( propertyName );

    if ( itProperty == SupportedProperties.end( ) )
    {
        ret = XError::UnknownProperty;
    }
    else
    {
        int     type = itProperty->second;
        int32_t intValue = 0;

        if ( type == TYPE_INT )
        {
            if ( sscanf( value.c_str( ), "%d", &intValue ) != 1 )
            {
                ret = XError::InvalidPropertyValue;
            }
        }

        if ( ret == XError::Success )
        {
            if ( propertyName == PROP_JPEG_QUALITY )
            {
                jpegQuality = static_cast<uint16_t>( intValue );
            }
            else if ( propertyName == PROP_MJPEG_RATE )
            {
                mjpegFrameRate = static_cast<uint16_t>( intValue );
            }
            else if ( propertyName == PROP_HTTP_PORT )
            {
                httpPort = static_cast<uint16_t>( intValue );
            }
            else if ( propertyName == PROP_VIEWERS )
            {
                viewersGroup = static_cast<uint16_t>( intValue );
            }
            else if ( propertyName == PROP_CONFIGURATORS )
            {
                configuratorsGroup = static_cast<uint16_t>( intValue );
            }
            else if ( propertyName == PROP_AUTH_DOMAIN )
            {
                authDomain = value;
            }
            else if ( propertyName == PROP_AUTH_METHOD )
            {
                authMethod = ( value == "basic" ) ? value : "digest";
            }
            else if ( propertyName == PROP_CUSTOM_WEB )
            {
                customWebContent = value;
            }
            else if ( propertyName == PROP_CAMERA_MONIKER )
            {
                cameraMoniker = value;
            }
            else if ( propertyName == PROP_CAMERA_TITLE )
            {
                cameraTitle = value;
            }
            else if ( propertyName == PROP_TIMESTAMP_OVR )
            {
                addTimestampOverlay = ( intValue != 0 );
            }
            else if ( propertyName == PROP_TITLE_OVR )
            {
                addCameraTitleOverlay = ( intValue != 0 );
            }
            else if ( propertyName == PROP_OVR_TEXT_COLOR )
            {
                StringToXargb( value, &overlayTextColor );
            }
            else if ( propertyName == PROP_OVR_BG_COLOR )
            {
                StringToXargb( value, &overlayBackgroundColor );
            }
            else if ( propertyName == PROP_CAMERA_WIDTH )
            {
                cameraWidth = static_cast<uint16_t>( intValue );
            }
            else if ( propertyName == PROP_CAMERA_HEIGHT )
            {
                cameraHeight = static_cast<uint16_t>( intValue );
            }
            else if ( propertyName == PROP_CAMERA_BPP )
            {
                cameraBpp = static_cast<uint16_t>( intValue );
            }
            else if ( propertyName == PROP_CAMERA_FPS )
            {
                cameraFps = static_cast<uint16_t>( intValue );
            }
            else if ( propertyName == PROP_REQUESTED_FPS )
            {
                requestedFps = static_cast<uint16_t>( intValue );
            }
            else if ( propertyName == PROP_MIN_TO_TRAY )
            {
                minimizeToSystemTray = ( intValue != 0 );
            }
            else if ( propertyName == PROP_WINDOW_ICON )
            {
                windowIcon = static_cast<uint16_t>( intValue );
            }
            else if ( propertyName == PROP_MAIN_WIN_X )
            {
                mainWindowX = intValue;
            }
            else if ( propertyName == PROP_MAIN_WIN_Y )
            {
                mainWindowY = intValue;
            }
            else if (propertyName == PROP_DEVICE_PREFERENCE)
            {
                devicePreference = value;
            }
        }
    }

    return ret;
}

// Get property of the object
XError AppConfig::GetProperty( const string& propertyName, string& value ) const
{
    XError                           ret        = XError::Success;
    map<string, int>::const_iterator itProperty = SupportedProperties.find( propertyName );

    if ( itProperty == SupportedProperties.end( ) )
    {
        ret = XError::UnknownProperty;
    }
    else
    {
        int     type     = itProperty->second;
        int32_t intValue = 0;

        if ( propertyName == PROP_JPEG_QUALITY )
        {
            intValue = jpegQuality;
        }
        else if ( propertyName == PROP_MJPEG_RATE )
        {
            intValue = mjpegFrameRate;
        }
        else if ( propertyName == PROP_HTTP_PORT )
        {
            intValue = httpPort;
        }
        else if ( propertyName == PROP_VIEWERS )
        {
            intValue = viewersGroup;
        }
        else if ( propertyName == PROP_CONFIGURATORS )
        {
            intValue = configuratorsGroup;
        }
        else if ( propertyName == PROP_AUTH_DOMAIN )
        {
            value = authDomain;
        }
        else if ( propertyName == PROP_AUTH_METHOD )
        {
            value = authMethod;
        }
        else if ( propertyName == PROP_CUSTOM_WEB )
        {
            value = customWebContent;
        }
        else if ( propertyName == PROP_CAMERA_MONIKER )
        {
            value = cameraMoniker;
        }
        else if ( propertyName == PROP_CAMERA_TITLE )
        {
            value = cameraTitle;
        }
        else if ( propertyName == PROP_TIMESTAMP_OVR )
        {
            intValue = ( addTimestampOverlay ) ? 1 : 0;
        }
        else if ( propertyName == PROP_TITLE_OVR )
        {
            intValue = ( addCameraTitleOverlay ) ? 1 : 0;
        }
        else if ( propertyName == PROP_OVR_TEXT_COLOR )
        {
            value = XargbToString( overlayTextColor );
        }
        else if ( propertyName == PROP_OVR_BG_COLOR )
        {
            value = XargbToString( overlayBackgroundColor );
        }
        else if ( propertyName == PROP_CAMERA_WIDTH )
        {
            intValue = cameraWidth;
        }
        else if ( propertyName == PROP_CAMERA_HEIGHT )
        {
            intValue = cameraHeight;
        }
        else if ( propertyName == PROP_CAMERA_BPP )
        {
            intValue = cameraBpp;
        }
        else if ( propertyName == PROP_CAMERA_FPS )
        {
            intValue = cameraFps;
        }
        else if ( propertyName == PROP_REQUESTED_FPS )
        {
            intValue = requestedFps;
        }
        else if ( propertyName == PROP_MIN_TO_TRAY )
        {
            intValue = ( minimizeToSystemTray ) ? 1 : 0;
        }
        else if ( propertyName == PROP_WINDOW_ICON )
        {
            intValue = windowIcon;
        }
        else if ( propertyName == PROP_MAIN_WIN_X )
        {
            intValue = mainWindowX;
        }
        else if ( propertyName == PROP_MAIN_WIN_Y )
        {
            intValue = mainWindowY;
        }
        else if ( propertyName == PROP_DEVICE_PREFERENCE )
        {
            value = devicePreference;
        }

        if ( type == TYPE_INT )
        {
            char buffer[32];

            sprintf( buffer, "%d", intValue );
            value = buffer;
        }
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
        if ( GetProperty( property.first, value ) )
        {
            properties.insert( pair<string, string>( property.first, value ) );
        }
    }

    return properties;
}
