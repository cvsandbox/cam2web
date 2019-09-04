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

#include "XVideoFrameDecorator.hpp"
#include "XImageDrawing.hpp"
#include <ctime>

using namespace std;

XVideoFrameDecorator::XVideoFrameDecorator( ) :
    cameraTitle( ),
    addTimestampOverlay( false ),
    addCameraTitleOverlay( false ),
    overlayTextColor( { 0xFF000000 } ),
    overlayBackgroundColor( { 0xFFFFFFFF } )
{

}

// Decorate the video frame coming from video source
void XVideoFrameDecorator::OnNewImage( const shared_ptr<const XImage>& image )
{
    string  overlay;

    if ( addTimestampOverlay )
    {
        std::time_t time = std::time( 0 );
        std::tm*    now = std::localtime( &time );
        char        buffer[32];

        sprintf( buffer, "%02d/%02d/%02d %02d:%02d:%02d", now->tm_year - 100, now->tm_mon + 1, now->tm_mday,
                                                          now->tm_hour, now->tm_min, now->tm_sec );

        overlay = buffer;
    }

    if ( ( addCameraTitleOverlay ) && ( !cameraTitle.empty( ) ) )
    {
        if ( !overlay.empty( ) )
        {
            overlay += " :: ";
        }

        overlay += cameraTitle;
    }

    if ( !overlay.empty( ) )
    {
        XImageDrawing::PutText( image, overlay, 0, 0, overlayTextColor, overlayBackgroundColor );
    }
}

// Get/Set camera title
string XVideoFrameDecorator::CameraTitle( ) const
{
    return cameraTitle;
}
void XVideoFrameDecorator::SetCameraTitle( const string& title )
{
    cameraTitle = title;
}

// Get/Set if timestamp should be overlayed on camera images
bool XVideoFrameDecorator::TimestampOverlay( ) const
{
    return addTimestampOverlay;
}
void XVideoFrameDecorator::SetTimestampOverlay( bool enabled )
{
    addTimestampOverlay = enabled;
}

// Get/Set if camera's title should be overlayed on its images
bool XVideoFrameDecorator::CameraTitleOverlay( ) const
{
    return addCameraTitleOverlay;
}
void XVideoFrameDecorator::SetCameraTitleOverlay( bool enabled )
{
    addCameraTitleOverlay = enabled;
}

// Get/Set overlay text color
xargb XVideoFrameDecorator::OverlayTextColor( ) const
{
    return overlayTextColor;
}
void XVideoFrameDecorator::SetOverlayTextColor( xargb color )
{
    overlayTextColor = color;
}

// Get/Set overlay background color
xargb XVideoFrameDecorator::OverlayBackgroundColor( ) const
{
    return overlayBackgroundColor;
}
void XVideoFrameDecorator::SetOverlayBackgroundColor( xargb color )
{
    overlayBackgroundColor = color;
}
