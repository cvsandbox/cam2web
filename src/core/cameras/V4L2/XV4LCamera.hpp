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

#ifndef XV4L_CAMERA_HPP
#define XV4L_CAMERA_HPP

#include <memory>

#include "IVideoSource.hpp"
#include "XInterfaces.hpp"

namespace Private
{
    class XV4LCameraData;
}

enum class XVideoProperty
{
    Brightness = 0,
    Contrast,
    Saturation,
    Hue,
    Sharpness,
    Gain,
    BacklightCompensation,
    RedBalance,
    BlueBalance,
    AutoWhiteBalance,
    HorizontalFlip,
    VerticalFlip
};

// Class which provides access to cameras using V4L2 API (Video for Linux, v2)
class XV4LCamera : public IVideoSource, private Uncopyable
{
protected:
    XV4LCamera( );

public:
    ~XV4LCamera( );

    static const std::shared_ptr<XV4LCamera> Create( );

    // Start video source so it initializes and begins providing video frames
    bool Start( );
    // Signal source video to stop, so it could finalize and clean-up
    void SignalToStop( );
    // Wait till video source (its thread) stops
    void WaitForStop( );
    // Check if video source is still running
    bool IsRunning( );

    // Get number of frames received since the start of the video source
    uint32_t FramesReceived( );

    // Set video source listener returning the old one
    IVideoSourceListener* SetListener( IVideoSourceListener* listener );

public: // Set of poperties, which can be set only when device is NOT running.
        // If it is running, then setting these properties is silently ignored.

    // Set/get video device
    uint32_t VideoDevice( ) const;
    void SetVideoDevice( uint32_t videoDevice );

    // Get/Set video size
    uint32_t Width( ) const;
    uint32_t Height( ) const;
    void SetVideoSize( uint32_t width, uint32_t height );

    // Get/Set frame rate
    uint32_t FrameRate( ) const;
    void SetFrameRate( uint32_t frameRate );

    // Enable/Disable JPEG encoding
    bool IsJpegEncodingEnabled( ) const;
    void EnableJpegEncoding( bool enable );

public:

    // Set the specified video property. The device does not have to be running. If it is not,
    // the setting will be cached and applied as soon as the device gets running.
    XError SetVideoProperty( XVideoProperty property, int32_t value );
    // Get current value if the specified video property. The device must be running.
    XError GetVideoProperty( XVideoProperty property, int32_t* value ) const;
    // Get range of values supported by the specified video property
    XError GetVideoPropertyRange( XVideoProperty property, int32_t* min, int32_t* max, int32_t* step, int32_t* def ) const;

private:
    Private::XV4LCameraData* mData;
};

#endif // XV4L_CAMERA_HPP

