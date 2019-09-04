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
#ifndef XVIDEO_FRAME_DECORATOR_HPP
#define XVIDEO_FRAME_DECORATOR_HPP

#include "IVideoSourceListener.hpp"

// Class aimed to put any sort of decorations on the images coming from video source (title, time, watermark/logo, etc)
class XVideoFrameDecorator : public IVideoSourceListener
{
public:
    XVideoFrameDecorator( );

    // New video frame notification
    void OnNewImage( const std::shared_ptr<const XImage>& image ) override;
    // Video source error notification
    void OnError( const std::string& /* errorMessage */, bool /* fatal */ ) override { }

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

private:
    std::string cameraTitle;
    bool        addTimestampOverlay;
    bool        addCameraTitleOverlay;
    xargb       overlayTextColor;
    xargb       overlayBackgroundColor;
};

#endif // XVIDEO_FRAME_DECORATOR_HPP
