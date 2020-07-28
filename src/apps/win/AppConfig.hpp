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

#ifndef APP_CONFIG_HPP
#define APP_CONFIG_HPP

#include <IObjectConfigurator.hpp>
#include <XImage.hpp>
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

    // Get/Set viewers group ID
    uint16_t ViewersGroup( );
    void SetViewersGroup( uint16_t groupId );

    // Get/Set configurators group ID
    uint16_t ConfiguratorsGroup( );
    void SetConfiguratorsGroup( uint16_t groupId );

    // Get/Set HTTP digest auth domain
    std::string AuthDomain( ) const;
    void SetAuthDomain( const std::string& authDomain );

    // Get/Set HTTP authentication method
    std::string AuthenticationMethod( ) const;
    void SetAuthenticationMethod( const std::string& method );

    // Get/Set path to the folder with custom web content
    std::string CustomWebContent( ) const;
    void SetCustomWebContent( const std::string& path );

    // Get/Set camera moniker string
    std::string CameraMoniker( ) const;
    void SetCameraMoniker( const std::string& moniker );

    // Get/Set camera title
    std::string CameraTitle( ) const;
    void SetCameraTitle( const std::string& title );

    // Get/Set if timestamp should be overlayed on camera images
    bool TimestampOverlay( ) const;
    void SetTimestampOverlay( bool enabled );

    // Get/Set if camera's title should be overlayed on its images
    bool CameraTitleOverlay( ) const;
    void SetCameraTitleOverlay( bool enabled );

    // Get/Set overlay text color
    xargb OverlayTextColor( ) const;
    void SetOverlayTextColor( xargb color );

    // Get/Set overlay background color
    xargb OverlayBackgroundColor( ) const;
    void SetOverlayBackgroundColor( xargb color );

    // Get/Set last used video resolution
    void GetLastVideoResolution( uint16_t* width, uint16_t* height, uint16_t* bpp, uint16_t* fps ) const;
    void SetLastVideoResolution( uint16_t  width, uint16_t  height, uint16_t  bpp, uint16_t  fps );

    // Get/Set last requested frame rate
    uint16_t LastRequestedFrameRate( ) const;
    void SetLastRequestedFrameRate( uint16_t fps );

    // Get/Set the flag inidicating that application should minimize to system tray
    bool MinimizeToSystemTray( ) const;
    void SetMinimizeToSystemTray( bool enabled );

    // Get/Set index of the window/tray icon to use
    uint16_t WindowIconIndex( ) const;
    void SetWindowIconIndex( uint16_t index );

    // Get/Set file name to store users' list in
    std::string UsersFileName( ) const;
    void SetUsersFileName( const std::string& fileName );

    // Get/Set main window's position
    int32_t MainWindowX( ) const;
    int32_t MainWindowY( ) const;
    void SetMainWindowXY( int32_t x, int32_t y );

    // Get user camaera device preference
    std::string DevicePreference( ) const;

public: // IObjectConfigurator interface

    virtual XError SetProperty( const std::string& propertyName, const std::string& value );
    virtual XError GetProperty( const std::string& propertyName, std::string& value ) const;

    virtual std::map<std::string, std::string> GetAllProperties( ) const;

private:
    uint16_t    jpegQuality;
    uint16_t    mjpegFrameRate;
    uint16_t    httpPort;
    uint16_t    viewersGroup;
    uint16_t    configuratorsGroup;
    std::string authDomain;
    std::string authMethod;
    std::string customWebContent;
    std::string cameraMoniker;
    std::string cameraTitle;
    std::string devicePreference;
    bool        addTimestampOverlay;
    bool        addCameraTitleOverlay;
    xargb       overlayTextColor;
    xargb       overlayBackgroundColor;
    uint16_t    cameraWidth;
    uint16_t    cameraHeight;
    uint16_t    cameraBpp;
    uint16_t    cameraFps;
    uint16_t    requestedFps;
    uint16_t    windowIcon;
    bool        minimizeToSystemTray;
    int32_t     mainWindowX;
    int32_t     mainWindowY;

    std::string usersFileName;
};

#endif // APP_CONFIG_HPP
