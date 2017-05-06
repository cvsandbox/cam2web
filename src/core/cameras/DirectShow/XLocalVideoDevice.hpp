/*
    cam2web - streaming camera to web

    BSD 2-Clause License

    Copyright (c) 2017, cvsandbox, cvsandbox@gmail.com
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef XLOCAL_VIDEO_DEVICE_HPP
#define XLOCAL_VIDEO_DEVICE_HPP

#include <XInterfaces.hpp>
#include <IVideoSource.hpp>
#include <vector>

#include "XDeviceName.hpp"
#include "XDeviceCapabilities.hpp"
#include "XDevicePinInfo.hpp"

namespace Private
{
    class XLocalVideoDeviceData;
};

enum class XVideoProperty
{
    Brightness = 0,
    Contrast,
    Hue,
    Saturation,
    Sharpness,
    Gamma,
    ColorEnable,
    WhiteBalance,
    BacklightCompensation,
    Gain
};

// Class which provides access to local video devices available through DirectShow interface
class XLocalVideoDevice : public IVideoSource, private Uncopyable
{
private:
    XLocalVideoDevice( const std::string& deviceMoniker );

public:
    ~XLocalVideoDevice( );

    static const std::shared_ptr<XLocalVideoDevice> Create( const std::string& deviceMoniker = std::string( ) );

    // Start video source so it initializes and begins providing video frames
    virtual bool Start( );
    // Signal video source to stop, so it could finalize and clean-up
    virtual void SignalToStop( );
    // Wait till video source stops
    virtual void WaitForStop( );
    // Check if video source (its background thread) is running or not
    virtual bool IsRunning( );
    // Get number of frames received since the the start of the video source
    virtual uint32_t FramesReceived( );

    // Set video source listener returning the old one
    virtual IVideoSourceListener* SetListener( IVideoSourceListener* listener );

    // Set device moniker for the video source (video source must not be running)
    bool SetDeviceMoniker( const std::string& moniker );
    // Set resolution and frame rate of the device (video source must not be running)
    bool SetResolution( const XDeviceCapabilities& resolution );
    // Set video input of the device
    void SetVideoInput( const XDevicePinInfo& input );

    // Get capabilities of the device (resolutions and frame rates)
    const std::vector<XDeviceCapabilities> GetCapabilities( );
    // Get available video pins of the device if any
    const std::vector<XDevicePinInfo> GetInputVideoPins( );
    // Check if device supports crossbar, which would allow setting video input pin
    bool IsCrossbarSupported( );

    // Get list of available video devices in the system
    static std::vector<XDeviceName> GetAvailableDevices( );

    // The IsRunning() reports if video source's background thread is running or not. However,
    // it does not mean the video device itself is running. When the thread is started, it
    // needs to perform certain configuration of the device and start it, which may or may not
    // succeed (device can be un-plugged, used by another application, etc). So the
    // IsDeviceRunning() reports status of the device itself.

    // Check if device is running
    bool IsDeviceRunning( ) const;
    // Wait till video configuration becomes available (milliseconds)
    bool WaitForDeviceRunning( uint32_t msec );

    // Check if video configuration is supported (device must be running)
    bool IsVideoConfigSupported( ) const;

    // Set the specified video property. The device does not have to be running. If it is not,
    // the setting will be cached and applied as soon as the device gets running.
    XError SetVideoProperty( XVideoProperty property, int32_t value, bool automatic = false );
    // Get current value if the specified video property. The device must be running.
    XError GetVideoProperty( XVideoProperty property, int32_t* value, bool* automatic = nullptr ) const;
    // Get range of values supported by the specified video property
    XError GetVideoPropertyRange( XVideoProperty property, int32_t* min, int32_t* max, int32_t* step, int32_t* default, bool* isAutomaticSupported ) const;

private:
    Private::XLocalVideoDeviceData* mData;
};

#endif // XLOCAL_VIDEO_DEVICE_HPP
